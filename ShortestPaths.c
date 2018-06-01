#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <Windows.h>

#define INF 9999

typedef struct vertex {
	char name[30];
	char parentName[30];
	int discover;
	int idx;
}Vertex;

typedef struct edge {
	int src;
	int dest;
	int weight;
}Edge;

typedef struct graph {
	int numV; //# of vertices
	int numE; //# of edges

	struct edge* edge; //array of edges - represent the graph(G)
	int weightMat[100][100]; //array of weights
	int resultMat[100][100]; //array of shortest paths from sources
	Vertex vertexMat[100]; //array of vertices
}Graph;

typedef struct heap {
	Vertex * priorityq[100]; //array of pointers to Graph.vertexMat[]
	int size;
}Heap;


Graph * createGraph(FILE * fp);
FILE * readFile();
void init_edge_weights(Graph * graphMap);
void init_single_source(Graph * graphMap, int source);
void Relax(int u, int v, int w, Graph * graphMap);
void printShortestPath(Graph * graphMap);
void Dijkstra(Graph * graphMap, int source);
int BellmanFord(Graph * graphMap, int source);
void Floyd(Graph * graphMap);

Vertex extract_min(Heap * pqueue);
void minHeapify(Heap * pqueue, int idx);
void swap(int temp, int idx, Heap * pqueue);
void pqueueInsert(Heap * pqueue, Vertex * v);


int main(int argc, char *argv[])
{
	FILE *fp = NULL;
	Graph * graphMap = NULL;
	int i = 0; 
	__int64 frequency;
	__int64 begin;
	__int64 end;
	double time_spent = 0.0F;

	QueryPerformanceFrequency((LARGE_INTEGER *) &frequency); //For keeping time

	fp = readFile(); //Establish input file stream

	graphMap = createGraph(fp);

	QueryPerformanceCounter((LARGE_INTEGER *) &begin);

	for (i = 0; i < graphMap->numV; i++) {
		Dijkstra(graphMap, i); //Run Djikstra's algorithm V times to find All-source shortest paths
	}

	QueryPerformanceCounter((LARGE_INTEGER *) &end);
	__int64 elapsed = end - begin;
	time_spent = (double)elapsed / (double)frequency;
	printf("It took %.3f seconds to compute shortest paths between cities with Dijkstra's algorithm as follows: \n\n", time_spent * 1000);
	printShortestPath(graphMap);

	memset(graphMap->resultMat, 0, sizeof(int) * 100 * 100);



	QueryPerformanceCounter((LARGE_INTEGER *) &begin);

	for (i = 0; i < graphMap->numV; i++) {
		int negativeCycle = BellmanFord(graphMap, i); //Run Bellman-Ford algorithm V times to find All-source shortest paths
		if (!negativeCycle) {
			fprintf(stderr, "Negative-weight cycles have been detected by Bellman-Ford algorithm.\nShortest paths could not be found.\n");
			exit(0);
		}
	}

	QueryPerformanceCounter((LARGE_INTEGER *) &end);
	elapsed = end - begin;
	time_spent = (double)elapsed / (double)frequency;
	printf("It took %.3f seconds to compute shortest paths between cities with Bellman-Ford algorithm as follows: \n\n", time_spent * 1000);
	printShortestPath(graphMap);

	memset(graphMap->resultMat, 0, sizeof(int) * 100 * 100); //Clear resultMat for the next algorithm



	QueryPerformanceCounter((LARGE_INTEGER *) &begin);
	Floyd(graphMap); //Run Floyd-Warshall algorithm to find All-source shortest paths

	QueryPerformanceCounter((LARGE_INTEGER *) &end);
	elapsed = end - begin;
	time_spent = (double)elapsed / (double)frequency;
	printf("It took %.3f seconds to compute shortest paths between cities with Floyd algorithm as follows: \n\n", time_spent * 1000);
	printShortestPath(graphMap);

}

FILE * readFile() {
	int c = 0;
	char str[20];
	FILE *fp = NULL;

	while (!fp) {
		printf("Enter the file name: ");
		if (fgets(str, sizeof(str), stdin) == NULL)
			fprintf(stderr, "Error. NULL received\n");
		else {
			char * pos;
			if ((pos = strchr(str, '\n')) != NULL) {//remove the trailing '\n'
				*pos = '\0';
			}
		}
		fp = fopen(str, "r");

		if (!fp) {
			perror("File open failed ");
			continue;
		}
		else {
			printf("<File open successful>\n");
			break;
		}
	}

	return fp;
}

Graph * createGraph(FILE * fp)
{
	Graph * graphMap = (Graph*)malloc(sizeof(Graph));
	int numEdge = 0;
	char mat[100] = {'\0'};
	char vertexMat[100][100] = { '\0' }; //List of vertices
	int row = 0, col = 0, i = 0, j = 0, numE = 0;
	const char* delim = "\t\n ";
	char * tok = NULL;

	if (!feof(fp)) {
		fgets(mat, sizeof(mat), fp); //Read a line (vertices)
		tok = strtok(mat, delim); //Tokenize the first line
		while (tok != NULL) {
			strcpy(vertexMat[row], tok); // Store the name of each vertex in vertexMat
			strcat(vertexMat[row], "\0"); 
			tok = strtok(NULL, delim);
			row++;
		}
	}

	for (i = 0; i < row; i++) {
		strncpy(graphMap->vertexMat[i].name, vertexMat[i], strlen(vertexMat[i]) + 1);
		graphMap->vertexMat[i].idx = i; // Store index of each vertex
	}

	memset(graphMap->weightMat, 0, sizeof(int) * 100 * 100); //Clear weightMat
	memset(graphMap->resultMat, 0, sizeof(int) * 100 * 100); //Clear resultMat
	graphMap->numV = row; //set number of vertices

	i = 0;
	j = 0;

	while (!feof(fp)) {
		char str[30];
		fscanf(fp, "%s", str); //Read each weight
		if (!strcmp("INF", str)) {
			strncpy(str, "9999\0", 5);
		}
		if(!isspace(str[0]) && (isdigit(str[0]) || str[0] == '-')){
			graphMap->weightMat[i][j] = atoi(str);

			j++;
			if (j == row) {
				i++;
				j = 0;
			}
		}
	}

	init_edge_weights(graphMap);

	return graphMap;
}

/*Initialize all the edges with the corresponding weights*/
void init_edge_weights(Graph * graphMap)
{
	int i = 0, j = 0, k = 0;
	//Number of edges in an undirected graph n(n-1)/2
	//Number of edges in a directed graph n(n-1)
	int maxnumEdge = graphMap->numV *(graphMap->numV - 1); //we are not guaranteed an undirected graph 
	graphMap->edge = (Edge*)malloc(sizeof(Edge) * maxnumEdge); //array of edges in graph
	graphMap->numE = 0;

	//Initialize the edges
	for (i = 0; i < graphMap->numV; i++) {
		for (j = 0; j < graphMap->numV; j++) {
			if (i != j && graphMap->weightMat[i][j] < INF) {
				graphMap->edge[k].src = i;
				graphMap->edge[k].dest = j;
				graphMap->edge[k].weight = graphMap->weightMat[i][j];
				k++;
			}
		}
	}
	graphMap->numE = k;
}

void init_single_source(Graph * graphMap, int source)
{
	int i = 0;
	for (i = 0; i < graphMap->numV; i++) {
		graphMap->vertexMat[i].discover = INF; //d[v] = INF
		memset(graphMap->vertexMat[i].parentName, '\0', sizeof(graphMap->vertexMat[i].parentName)); //p[v] = NIL
	}

	graphMap->vertexMat[source].discover = 0;
}

void Relax(int u, int v, int w, Graph * graphMap)
{
	if (graphMap->vertexMat[v].discover > graphMap->vertexMat[u].discover + w) {
		graphMap->vertexMat[v].discover = graphMap->vertexMat[u].discover + w;
		strncpy(graphMap->vertexMat[v].parentName, graphMap->vertexMat[u].name, strlen(graphMap->vertexMat[u].name) + 1);
		//printf("%s relaxed %s (d: %d)\n", graphMap->vertexMat[v].parentName, graphMap->vertexMat[v].name, graphMap->vertexMat[v].discover);
	}
}

void Dijkstra(Graph * graphMap, int source)
{
	int i = 0, j = 0, k = 0, l = 0;
	init_single_source(graphMap, source);
	Heap * priority_q = (Heap*)malloc(sizeof(Heap));
	priority_q->size = 0;
	Vertex * setMinPath = (Vertex*)malloc(sizeof(Vertex) * (graphMap->numV)); // Set(S) for vertices selected by ExtractMin 

	for (i = 0; i < graphMap->numV; i++) //Insert the vertices into the priority queue
	{
		pqueueInsert(priority_q, &(graphMap->vertexMat[i]));
	}

	//ExtractMin and do a BFS traversal on the graph
	i = 0;
	while (priority_q->size > 0)
	{
		for (l = (priority_q->size - 1) / 2; l >= 0; l--) {
			minHeapify(priority_q, l);
		}

		setMinPath[i] = extract_min(priority_q); // S U {u}

		int vertex = setMinPath[i].idx;
		for (k = 0; k < graphMap->numV; k++) {
			if(graphMap->weightMat[vertex][k] != 0 || graphMap->weightMat[vertex][k] != INF) //BFS - if there is an edge, RELAX(u, v, w)
				Relax(vertex, k, graphMap->weightMat[vertex][k], graphMap);// RELAX(u,v,w) each edge
		}
		i++;
	}

	for (i = 0; i < graphMap->numV; i++) {
		graphMap->resultMat[source][i] = graphMap->vertexMat[i].discover;
	}
}
int BellmanFord(Graph * graphMap, int source)
{
	int i = 0, j = 0;

	init_single_source(graphMap, source);
	
	for (i = 0; i < graphMap->numV-1; i++) {
		for (j = 0; j < graphMap->numE; j++) {
			Relax(graphMap->edge[j].src, graphMap->edge[j].dest, graphMap->edge[j].weight, graphMap);// RELAX(u,v,w) each edge
		}
	}

	/*Check for the negative weight cycle*/
	for (i = 0; i < graphMap->numE; i++) {
		int u = graphMap->edge[i].src;
		int v = graphMap->edge[i].dest;
		if (graphMap->vertexMat[v].discover > graphMap->vertexMat[u].discover + graphMap->edge[i].weight) { // d[v] > d[u] + w
			return 0; //return if negative weight cycle exists
		}
	}

	for (i = 0; i < graphMap->numV; i++) {
		graphMap->resultMat[source][i] = graphMap->vertexMat[i].discover;
	}

	return 1;
}
void Floyd(Graph * graphMap)
{
	int i = 0, j = 0, k = 0;

	/*graphMap->weightMat is already initialized
	with corresponding edge weights*/

	for (k = 0; k < graphMap->numV; k++) {
		for (i = 0; i < graphMap->numV; i++) {
			for (j = 0; j < graphMap->numV; j++) {
				if (graphMap->weightMat[i][k] + graphMap->weightMat[k][j] < graphMap->weightMat[i][j]) {
					graphMap->weightMat[i][j] = graphMap->weightMat[i][k] + graphMap->weightMat[k][j];
				}
				else {
					graphMap->weightMat[i][j] = graphMap->weightMat[i][j];
				}
			}
		}
	}
	for (i = 0; i < graphMap->numV; i++) {
		for(j = 0; j < graphMap->numV; j++)
		graphMap->resultMat[i][j] = graphMap->weightMat[i][j];
	}

}

void printShortestPath(Graph * graphMap) 
{
	int i = 0, j = 0;
	printf("%-12s", " ");
	for (i = 0; i < graphMap->numV; i++) {
		printf("%-12s", graphMap->vertexMat[i].name);
	}
	printf("\n");

	for (i = 0; i < graphMap->numV; i++) {
		printf("%-12s", graphMap->vertexMat[i].name);
		for (j = 0; j < graphMap->numV; j++) {
			printf("%-12d", graphMap->resultMat[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

void minHeapify(Heap * pqueue, int idx)
{
	int temp = idx;

	if (pqueue->size < (idx+1) * 2) { //no child node; heapify is complete
		return;
	}

	if (pqueue->size == (idx + 1) * 2) { //only left child exists
		if (pqueue->priorityq[idx]->discover > pqueue->priorityq[(idx * 2) + 1]->discover)
			temp = (idx * 2) + 1;
	}
	else { //has both children (left and right)
		if (pqueue->priorityq[(idx * 2) + 1]->discover < pqueue->priorityq[(idx * 2) + 2]->discover) { //apply to the one with the smaller value
			if (pqueue->priorityq[idx]->discover > pqueue->priorityq[(idx * 2) + 1]->discover)
				temp = (idx * 2) + 1;
		}
		else {
			if (pqueue->priorityq[idx]->discover > pqueue->priorityq[(idx * 2) + 2]->discover)
				temp = (idx * 2) + 2;
		}
	}

	if(temp != idx) { //if temp has been altered, more heapify should be done
		swap(temp, idx, pqueue); //swap for heapify
		minHeapify(pqueue, temp);
	}
}

Vertex extract_min(Heap * pqueue)
{
	Vertex root;
	if (pqueue->size <= 0) {
		printf("No vertex in the priority queue. Try again.\n");
	}
	else if (pqueue->size == 1) {
		pqueue->size -= 1;
		return (*(pqueue->priorityq[0]));
	}

	root = *(pqueue->priorityq[0]);
	pqueue->priorityq[0] = pqueue->priorityq[pqueue->size - 1]; // Switch last vertex with root
	pqueue->size--;
	minHeapify(pqueue, 0);

	return root;
}

void pqueueInsert(Heap * pqueue, Vertex * v)
{
	if (pqueue->priorityq == NULL || pqueue == NULL) {
		fprintf(stderr, "No priority queue allocated. Try again.\n");
		return;
	}
	
	pqueue->priorityq[pqueue->size] = v;
	pqueue->size++;
}

void swap(int temp, int idx, Heap * pqueue)
{
	/*swap the address of vertexMats (pointed to by Vertex * priorityq[100])*/
	Vertex * tmp = pqueue->priorityq[temp];
	pqueue->priorityq[temp] = pqueue->priorityq[idx];
	pqueue->priorityq[idx] = tmp;
}