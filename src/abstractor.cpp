 /* @file abstractor.cpp
 * @author Can Koban
 *
 * @brief In this project, a multithreaded process is created and synchronization is ensured with mutexes.
 * Project aims to calculate Jaccard Similarty Ratio and find the summary of the abstract.txt files with 
 * the parallel execution of created threads.
 */  
#include <iostream> 
#include <fstream>
#include <string>
#include <set>
#include <vector>
#include <queue>
#include <pthread.h>
#include <unistd.h>
#include <iomanip>
using namespace std;
const unsigned MAX_LENGTH = 1024;
//Following three data structures are used as global variables that threads write to and read from.
queue<std::string> abstracts; //abstract names are popped from this queue inside a critical section.
string query; //Words for query line is stored in it.
queue<string> calculatingQueue; //"Thread X is calculating abstract_Y.txt" type sentences are stored in it. 
pthread_mutex_t mutex;
pthread_mutex_t mutex2;

//Following result class is to define results in a compact form.
class Result    
 {    
     public:  
     float jaccardRatio;
     string abstractName;
     string sentences;  
 };   

 bool operator<(const Result& r1, const Result& r2)
{
    return r1.jaccardRatio < r2.jaccardRatio;
}
//Results are added to priority queue and sorted according to jaccard ratio in the decreasing order.
priority_queue<Result> pqResult;
/*Following two functions: function() and startThread() are for threads. Main function calls startThread with the pthread_create()
* Then inside the startThread() each thread pops abstract name and continues to execute by calling function(). Popping mechanism is
* preserved by mutexes. Note that threads continue in parallel and while abstracts queue is not empty threads don't join.
*/ 
void function(string abstractName,string sentQuery){
	set<string> querySet;
	vector<string> queryVector;
	vector<string> queryVector2;
	//Getting input words. Since no thread try to reach same file at the same time.
	string path = "../abstracts/" + abstractName;
	string str;
	stringstream check2(sentQuery);
	while(getline(check2,str,' ')){  //Words are seperated with the space
		if (str.find("\n") != std::string::npos) {
    		str.pop_back(); //If the last character of the word is '\n' remove it.
		}
			querySet.insert(str); //Add the word to set and a vector.
			queryVector.push_back(str);
		}		
		querySet.erase("\n");//Erase \n element from set.
		querySet.erase("");//Erase empty element from set.

	ifstream instream(path);
	Result result; //result local object is created for each thread to store results.
	result.abstractName=abstractName; //File: ... in the output file.

    set<string> wordSet;
    queue<string> wordQueue; 
	if (instream.is_open()){
    string line;
    string srg;
	while(getline(instream,line)){
		stringstream check1(line);
		while(getline(check1,srg,' ')){//Words are seperated with the spac
			wordSet.insert(srg);
			wordQueue.push(srg);
		}
	}
	wordSet.erase("\n");//Erase \n element from set.
	wordSet.erase("");//Erase empty element from set.
	instream.close();}
	else {cout <<"File opening is fail.";} 

	//Sentences in the abstract files are stored in the sentQueue.
	string sent;
	queue<string> sentQueue;
	while(!wordQueue.empty()){
		if(wordQueue.front()!="."){
			if(wordQueue.front()!="\n"){ 
				sent+=wordQueue.front()+" ";
				wordQueue.pop();
			}
			else{ 
				wordQueue.pop();
			}
		}
		else{
			wordQueue.pop();
			sentQueue.push(sent);
			sent.clear(); 
		}
	}

	//If common word between queryVector and in the sentences are found, sentences are added each other and total string stored in the result object.
	std::string sentList; 
	while(!sentQueue.empty()){
		for(vector<string>::iterator itr=queryVector.begin();itr!=queryVector.end();++itr){
			int found=sentQueue.front().find(*itr);
			if (found != std::string::npos){
				string sentFound=sentQueue.front();
				string strItr=*itr;
				//Checking that the word same comletely with the common word in query.
				//To do that, I checked whether the front and back of the word is ' ' or not.
				//Of course I checked the condition where common word is located at 0.
				if (sentFound[found+strItr.length()]==' ' && found!=0 && sentFound[found-1]==' '){
					sentList.append(sentFound+". ");
					break;
				}else if (sentFound[found+strItr.length()]==' '&& found==0){
					sentList.append(sentFound+". ");
					break;
				} 
			}
		}
		sentQueue.pop();
	}
	result.sentences=sentList;
 	
 	//Then, Jaccard ratio is calculating by counting number of words in common between the sets querySet and wordSet.
	int common=0;
	for(auto itr=querySet.begin();itr!=querySet.end();++itr){
		if (wordSet.count(*itr)==1){
			common++; 
		}
	}
	int unionSize=wordSet.size()+(querySet.size()-common);
	float jaccardRatio=(float)(common)/(float)unionSize;
	result.jaccardRatio=jaccardRatio;

	pqResult.push(result);//All of the results are pushed to the globale variable pqResult and popped in the main.
}

/*startThread() function takes one argument which is the name of the thread. It pops one abstract name and writes 
* "Thread X is calculating abstract_Y.txt" to  calculating queue to write to the output file in main function.
* Then, starts function() with the only parameter that abstract name. 
*/
void* startThread(void* arg) {
	char letter = *(char*)arg; 
    string stringLetter(1, letter);
    std::string abstract;
    string sentQuery=query;
	if (abstracts.empty()){
		pthread_exit(NULL);  //If there is nothing to do, thread is created for nothing. Kill it.
	}
	while (!abstracts.empty()) {
	    if (!abstracts.empty()){
	    	abstract=abstracts.front();
	    pthread_mutex_lock(&mutex);
	    	abstracts.pop();			//Critical section to prevent two or more thread pop() at the same time
    	pthread_mutex_unlock(&mutex);
	    }else{ 
	    	break; 
	    }
       	string sentCalculating= "Thread " + stringLetter+ " is calculating "+abstract;
	    pthread_mutex_lock(&mutex2);
       	calculatingQueue.push(sentCalculating);	//Critical section to prevent two or more thread push() at the same time  
        pthread_mutex_unlock(&mutex2);	
        function(abstract,sentQuery);
	}
	pthread_exit(NULL);
} 
 
int main(int argc, char const *argv[]) {
	int T,A,N;
	char query_temp[MAX_LENGTH];

	//Input reading starts.
	const char * path=argv[1];
	FILE *fp = fopen(path, "r");

    if (fp == NULL)     
    {
        printf("Error: could not open file");
        return 1;
    }

    char buffer[MAX_LENGTH];

    //T, A and N are read.
    fgets(buffer, MAX_LENGTH, fp);
    sscanf(buffer,"%d %d %d",&T,&A,&N);

    //Query is read and assigned to global variable.
	fgets(query_temp, MAX_LENGTH, fp);
	query=query_temp;
 	
 	//To name threads alphabet queue is formed.
	queue<char>	alphabet;
	alphabet.push('A');alphabet.push('B');alphabet.push('C');alphabet.push('D');alphabet.push('E');alphabet.push('F');alphabet.push('H');
	alphabet.push('I');alphabet.push('J');alphabet.push('K');alphabet.push('L');alphabet.push('M');alphabet.push('N');alphabet.push('O');
	alphabet.push('P');alphabet.push('Q');alphabet.push('R');alphabet.push('S');alphabet.push('T');alphabet.push('U');alphabet.push('V');
	alphabet.push('W');alphabet.push('X');alphabet.push('Y');alphabet.push('Z');
   
	//Abstract names are red and added to global variable abstracts queue
    for (int i = 0; i < A; ++i){
		fgets(buffer, MAX_LENGTH, fp);
		string abstract=buffer;
		if (abstract.find("\n") != std::string::npos) {
    		abstract.pop_back();
		}
		abstracts.push(abstract);
	}
	fclose(fp);

	//T number of threads are created and two mutex is used in this project.
	pthread_t th[T];
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutex2, NULL);
    //Create threads with startThread function and send the name of the thread in the creation order.
    int i;
    for (i = 0; i < T; i++) {
	    char* letter = (char*) malloc(sizeof(char));
        *letter = alphabet.front();
        alphabet.pop();
        if (pthread_create(&th[i], NULL, &startThread, letter) != 0) {
            perror("Failed to create the thread");
        }
    }
    //Thrads will wait to join until abstracts queue is empty and their sequential exection ends.
    for (i = 0; i < T; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutex2);


    //Outputting the results starts here.
	ofstream outputStream; 
	outputStream.open(argv[2]);
  if (outputStream.is_open())
  {
    while (!calculatingQueue.empty()){
    	outputStream<<calculatingQueue.front()<<endl;
    	calculatingQueue.pop();
    }
  }
  else cout << "Unable to open file";

    outputStream<<"###"<<endl;
    for (int i = 0; i < N; ++i){
    	outputStream<<"Result "+to_string(i+1)+":"<<endl; 
    	outputStream<<"File: "+pqResult.top().abstractName<<endl;
    	outputStream<<"Score: "<<std::fixed<<setprecision(4)<<pqResult.top().jaccardRatio<<endl;
    	outputStream<<"Summary: "+pqResult.top().sentences<<endl;  
    	outputStream<<"###"<<endl;   
    	pqResult.pop();
    } 
        outputStream.close();
	  
return 0;
}