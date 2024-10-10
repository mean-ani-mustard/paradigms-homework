using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

struct actorKey{
  const void * file;
  const string *playerName;
  
};
struct movieKey{
  const void *file;
  const film *movie;

};

int actorCmp (const void *vp1, const void *vp2){
  actorKey* myKey = (actorKey*)vp1;
  string actorName;
  int b = *(int*)vp2;
  char * name0 = (char*) myKey->file +b;
  actorName = name0;
  if(actorName==*(myKey->playerName)){
    return 0;
  }else if(*(myKey->playerName) < actorName ){
    return -1;
  }
  return 1;
}

int movieCmp (const void *vp1, const void *vp2){
  movieKey* myKey = (movieKey*) vp1;
  film movie0;
  char * a = (char*) myKey->file;
  int b = *(int*)vp2;
  char * title0 = a+b;
  string mtitle = title0;
  movie0.title = mtitle;
  int premierYear = 1900 + *(a + b + mtitle.size()+1);
  
  movie0.year = premierYear;

  if(*(myKey->movie) == movie0){
    return 0;
  }else if(*(myKey->movie) < movie0){
    return -1;
  }
  return 1;
  
}

// you should be implementing these two methods right here... 
bool imdb::getCredits(const string& player, vector<film>& films) const {

   actorKey myPlayer;
   myPlayer.file=actorFile;
   myPlayer.playerName = &player;

   int actorsN = *(int *) actorFile;
   int *found = (int *) bsearch(&myPlayer, (int*) actorFile + 1, actorsN, sizeof(int), actorCmp);
   
   if(found != NULL){
    int additional = (player.size() % 2 == 0) ? 1 : 0;
    int locat = *found + player.size()+ 1+ additional;
    short creditCount = *((short*)((char*)actorFile + locat));
    locat += sizeof(short);
    if((player.size()+1+additional+2)%4 !=0){
      locat+= 2;
    }
    
    for (short i = 0; i < creditCount; i++){
      film starredIn;
      int movieLocat = *(int*)((char*)actorFile + locat + i*sizeof(int));
      char* movieName = (char *)movieFile + movieLocat;
      string mtitle = movieName;
      movieName = movieName + mtitle.size() + 1;
      starredIn.title = mtitle;
      //movieName++;
      int movieYear = 1900 + *movieName;
      starredIn.year = movieYear;
      films.push_back(starredIn);
      

    }

    return true;
   }
   
   return false; 


}
bool imdb::getCast(const film& movie, vector<string>& players) const { 
  
  movieKey myMovie;
  myMovie.file = movieFile;
  myMovie.movie = &movie;
  int filmsN = *(int *) movieFile;
  int * found = (int *) bsearch(&myMovie, (int *)movieFile + 1, filmsN, sizeof(int), movieCmp);

  if(found != NULL){
    string movieTitle = movie.title;
    int additional = ((movieTitle.size()+1) % 2 == 0) ? 1 : 0;
    int locat = *found + movieTitle.size()+2+additional;
    short memberCount = *((short*)((char *)movieFile + locat));
    locat += sizeof(short);
    if((movieTitle.size()+2+additional+2)%4 !=0){
      locat+= 2;
    }
    for (short i = 0; i < memberCount; i++){
      string castMember;
      int castLocat = *(int*)((char*)movieFile + locat + i*sizeof(int));
      char* memberName = (char *)actorFile + castLocat;
      castMember = memberName;


      players.push_back (castMember);
    }
    
    return true;
   }

  
  
  return false; 
  

}



imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
