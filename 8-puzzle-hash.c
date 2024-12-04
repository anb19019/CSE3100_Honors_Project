#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "linked-list.h"

#define N 3
//#define NUM_THREADS 3
#define HASH_SIZE 10000

//Shared queue structure for managing nodes
typedef struct {
    node *head, *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int found;
} shared_queue;

//Hash table structure for storing visited nodes
typedef struct {
    node *table[HASH_SIZE];
    pthread_mutex_t mutex;
} hash_table;

//Function to compute a hash value for a given puzzle
int hash_puzzle(int puzzle[N][N]){
    int hash_value = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++){
            hash_value = hash_value * 10 + puzzle[i][j];
        }
    }
    return hash_value;
};

//Function to add node to the visited hash table
void add_to_visited(hash_table *h_table, node *current_node){
    int hash_value = hash_puzzle(current_node->puzzle) % HASH_SIZE;
    node *new_node = create_node(current_node->puzzle, current_node->x, current_node->y, current_node->x, current_node->y, current_node->depth + 1, current_node->parent);
    pthread_mutex_lock(&h_table->mutex);
    add_first(&(h_table->table[hash_value]), new_node);
    pthread_mutex_unlock(&h_table->mutex);
}

//Function to free the memory allocated for the hash table
void free_table(hash_table *h_table){
    for (int i = 0; i < HASH_SIZE; i++){
        //Remove all nodes in each bucket
        remove_all(&(h_table->table[i]));
    }
}

//Strucure for thread arguments
typedef struct{
    int id;
    shared_queue *queue;
    hash_table *visited;
    int goal[N][N];
} thread_arg_t;

//Function to check id two puzzles are equal
int is_equal(int puzzle1[N][N], int puzzle2[N][N]){
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (puzzle1[i][j] != puzzle2[i][j]){
                return 0;
            }
        }
    }
    return 1;
}

//Function to fancy display the state ot the puzzle
void display_state(int puzzle[N][N]){
    printf("****************\n");
    for (int i = 0; i < N; i++) { 
        for (int j = 0; j < N; j++) { 
            printf("%d ", puzzle[i][j]); 
        } 
        printf("\n"); 
    } 
    printf("****************\n");
}

//Function to prin tthe path from the inital state to the current state
void printPath(node* node) {
    if (node == NULL) {
        return;
    }
    //Recursively print the ptath of parent nodes
    printPath(node->parent);
    printf("Step %d :\n", node->depth);
    //Display state at current depth
    display_state(node->puzzle);
}

//Function to free the memory allocated for the tree of nodes
void free_tree(node *n) {
    if (n == NULL){
        return;
    }

    for (int i = 0; i < n->num_children; i++){
        //Recursively free child nodes
        free_tree(n->children[i]);
    }

    //Free the current node
    free(n);
}

// Function to check if a puzzle has been visited already
int is_visited(hash_table *h_table, int puzzle[N][N]){
    int hash_value = hash_puzzle(puzzle) % HASH_SIZE;
    pthread_mutex_lock(&h_table->mutex);
    node *node = h_table->table[hash_value];
    while (node != NULL){
        if (is_equal(node->puzzle, puzzle)){
            pthread_mutex_unlock(&h_table->mutex);
            return 1;
        }
        node = node->next;
    }
    pthread_mutex_unlock(&h_table->mutex);
    return 0;
}

//Function to add a child node to the parent node
void add_child(node *parent, node *child) {
    parent->children[parent->num_children] = child;
    parent->num_children++;
}

//Function to solve the given puzzle configuration
void *bfs_thread(void *threadarg){

    //Initailize variables from thread arguments
    thread_arg_t *my_data = (thread_arg_t*) threadarg;
    int id = my_data->id;
    shared_queue *queue = my_data->queue;
    hash_table *visited = my_data->visited;
    int goal[N][N];
    memcpy(goal, my_data->goal, sizeof(goal));

    //Define possible moves (down , up, right, left)
    int row[] = {1, -1, 0, 0};
    int column[] = {0, 0, 1, -1};

    //Loop until the solution is found
    while (!queue->found){
        pthread_mutex_lock(&queue->mutex);
        //Wait for items to be added to the queue
        while ((queue->count == 0) && (!queue->found)){
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }

        //Exit if solution found in the meantime
        if (queue->found) { 
            pthread_mutex_unlock(&queue->mutex);
            pthread_exit(NULL); 
        }

        //Remove the first node from queue for processing
        node *current_node = remove_first(&queue->head, &queue->tail);
        queue->count--;

        pthread_mutex_unlock(&queue->mutex);

        //Check if the current node is the goal state
        if(is_equal(current_node->puzzle, goal)) {
            pthread_mutex_lock(&queue->mutex);
            queue->found = 1;
            pthread_cond_broadcast(&queue->cond);
            pthread_mutex_unlock(&queue->mutex);
            printf("Solution # 1\n");
            printPath(current_node);
            pthread_exit(NULL);
        }


        // Generate the child nodes for each possible move
        for (int i = 0; i < 4; i++) {
            int newX = current_node->x + row[i];
            int newY = current_node->y + column[i];
            if (newX >= 0 && newX < N && newY >= 0 && newY < N) { 
                node *child = create_node(current_node->puzzle, current_node->x, current_node->y, newX, newY, current_node->depth + 1, current_node);
                //Check if the child puzzle has been visited
                if (!is_visited(visited, child->puzzle)){
                    //Add to the visited hash table
                    add_to_visited(visited, child);
                    //Add to the current nodes children
                    add_child(current_node, child);
                    pthread_mutex_lock(&queue->mutex);
                    //Add to the queue
                    add_last(&queue->head, &queue->tail, child); 
                    queue->count++; 
                    pthread_cond_broadcast(&queue->cond);
                    pthread_mutex_unlock(&queue->mutex);
                } else {
                    //Free the memory if the puzzle has been visited
                   free(child);
                }
            }
        }
    }

    return NULL;

}

// Main program function
int main(int argc, char *argv[]) {

    if(argc != 2)
	{
		printf("Usage: %s thread_num \n", argv[0]);
		return -1;
	}
	int num_threads = atoi(argv[1]);

    //Inital puzzle state
    int start[N][N] = { 
        {8, 7, 6}, 
        {5, 4, 3}, 
        {2, 1, 0} 
    }; 
    
    //Goal puzzle state
    int goal[N][N] = { 
        {1, 2, 3}, 
        {4, 5, 6}, 
        {7, 8, 0}
    };

    //Allocate and initalize shared queue
    shared_queue *queue = (shared_queue *)malloc(sizeof(shared_queue)); 
    if (queue == NULL) { 
        perror("Cannot allocate memeory.\n");
        return -1;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    queue->found = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);

    //Allocate and initialized the hash table of visited nodes
    hash_table *visited = (hash_table *)malloc(sizeof(hash_table)); 
    if (visited == NULL) { 
        perror("Cannot allocate memory for hash table.\n"); 
        return -1; 
    }
    for (int i = 0; i < HASH_SIZE; i++) { 
        visited->table[i] = NULL; 
    } 
    pthread_mutex_init(&visited->mutex, NULL);

    // Create the root node for the inital state
    node *root = create_node(start, 2, 2, 2, 2, 0, NULL);
    add_last(&queue->head, &queue->tail, root); 
    queue->count++;

    //Create and initalize thread data
    pthread_t threads[num_threads];
    thread_arg_t thread_data_array[num_threads];

    int rc, i;
    for (i = 0; i < num_threads; i++){
        thread_data_array[i].id = i;
        thread_data_array[i].queue = queue;
        thread_data_array[i].visited = visited;
        memcpy(thread_data_array[i].goal, goal, sizeof(goal));
        rc = pthread_create(&threads[i], NULL, bfs_thread, &thread_data_array[i]);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    // Join threads after they complete exectution
    for (i = 0; i < num_threads; i++){
        rc = pthread_join(threads[i], NULL);
        if( rc ){
            printf("ERROR; return code from pthread_join() is %d\n", rc);
        }
    }
    
    // Destory mutexes and condition variables
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    pthread_mutex_destroy(&visited->mutex);

    //Free all allocated memory
    free_tree(root);
    free_table(visited);
    free(visited);
    free(queue);

    return 0;
}