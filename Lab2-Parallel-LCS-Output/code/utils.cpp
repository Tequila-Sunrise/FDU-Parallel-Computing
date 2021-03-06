#include <cstdlib>
#include <cstdio>

#include "utils.h"

#ifndef ANTICANONICAL_DEBUG
#define NDEBUG
#endif
#include <cassert>

int max(const int a, const int b) {
    return (a > b) ? a : b;
}

int min(const int a, const int b) {
    return (a > b) ? b : a;
}

int naive_lcs_2d(const char* A, const char* B, int M, int N) {
    int* dp = (int*)calloc(M * N, sizeof(int));

    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {
            int up = (i > 0) ? dp[(i - 1) * N + j] : 0;
            int left = (j > 0) ? dp[i * N + j - 1] : 0;
            int upleft = (i > 0 && j > 0) ? dp[(i - 1) * N + j - 1] : 0;

            if (A[i] == B[j])
                dp[i * N + j] = upleft + 1;
            else
                dp[i * N + j] = max(up, left);
        }
    }
    return dp[M * N - 1];
}

cell_t* lcs_anticanonical(const char* A, const char* B, int M, int N) {
    // init
    const int RLEN = N + 1, CLEN = M + 1;
    const int NEXT_LIST_SIZE = (M < N) ? M : N;
    cell_t* table = (cell_t*)calloc((M + 1) * (N + 1), sizeof(cell_t));

    for (int i = 0; i <= M; i++) {
        int j = 0;
        SET_STRUCT(cell_t, table[i * RLEN + j], i, 0, 0, NULL, NULL, NULL, NULL, false, '\0', 0, NULL);
    }

    for (int j = 0; j <= N; j++) {
        int i = 0;
        SET_STRUCT(cell_t, table[i * RLEN + j], 0, j, 0, NULL, NULL, NULL, NULL, false, '\0', 0, NULL);
    }

    // naive LCS algorithm
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= N; j++) {
            int rank;
            char match;

            if (A[i - 1] == B[j - 1]) {
                int upleft = table[(i - 1) * RLEN + j - 1].rank;
                rank = upleft + 1;
                match = A[i - 1]; // == B[j-1] 
            }
            else {
                int up = table[(i - 1) * RLEN + j].rank;
                int left = table[i * RLEN + j - 1].rank;
                rank = (up > left) ? up : left;
                match = '\0';
            }
            SET_STRUCT(cell_t, table[i * RLEN + j], i, j, rank, NULL, NULL, NULL, NULL, false, match, 0, NULL);
        }
    }

    // All-prefixes-LCSs-graph construction
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= N; j++) {
            cell_t* tail, * pretail, * head, * next;
            int next_num;
            cell_t** next_list;

            tail = pretail = head = next = NULL;
            next_num = 0;
            next_list = (cell_t**)calloc(NEXT_LIST_SIZE, sizeof(cell_t*));

            cell_t& cur = table[i * RLEN + j];
            const int rank = cur.rank;
            const char match = cur.match;

            // match
            if (match != '\0') {
                tail = &cur;

                next_num = 1;
                next_list[0] = &table[(i - 1) * RLEN + (j - 1)];
            }
            // clash
            else {
                if (rank <= 0) continue;

                cell_t& up = table[(i - 1) * RLEN + j];
                cell_t& left = table[i * RLEN + j - 1];

                assert(rank == (up.rank > left.rank) ? up.rank : left.rank);

                if (up.rank > left.rank) {
                    tail = up.tail;
                }
                else if (up.rank < left.rank) {
                    tail = left.tail;
                }
                else {
                    assert(up.tail != NULL && left.tail != NULL);
                    tail = up.tail;
                    pretail = (up.tail->i == left.tail->i) ? left.pretail : left.tail;

                    int pretail_j = (pretail == NULL) ? -1 : pretail->j;

                    if (pretail_j == tail->j) {
                        tail = pretail;
                    }
                    else {
                        if (pretail != NULL)
                            pretail->next = tail;
                        head = (left.head != NULL) ? left.head : pretail;
                    }
                }

                cell_t* ptr = (head == NULL) ? tail : head;
                while (ptr != NULL) {
                    next_list[next_num++] = ptr;

                    if (ptr->i <= tail->i || ptr->j >= tail->j) {
#ifdef ANTICANONICAL_DEBUG
                        if ((ptr != tail) || (ptr->i != tail->i) || (ptr->j != tail->j))
                            printf("Warning: end at the cell (%d,%d) but not tail(%d,%d).\n", ptr->i, ptr->j, tail->i, tail->j);
#endif
                        break;
                    }

                    if (ptr == tail)
                        ptr = NULL;
                    else if (ptr == pretail)
                        ptr = tail;
                    else ptr = ptr->next;
                }
            }

            assert(next_num <= NEXT_LIST_SIZE);

            SET_STRUCT(cell_t, cur, i, j, rank, tail, pretail, head, next, false, match, next_num, next_list);

#ifdef ANTICANONICAL_DEBUG
            printf("(%d,%d)[rank=%d, match=%c, next=%d]: ", i, j, rank, match, next_num);
            for (int k = 0; k < next_num; k++) {
                cell_t* c = next_list[k];
                printf("(%d,%d) ", c->i, c->j);
            }
            printf("\n");
#endif
        }
    }
    cell_t* entry = &table[RLEN * CLEN - 1];
    return entry;
}

// A utility function to create a new linked lcs node
lcsNode* newNode(cell_t* cell) {
    lcsNode* temp = (lcsNode*)malloc(sizeof(lcsNode));
    temp->cell = cell;
    temp->parent = NULL;
    temp->next = NULL;
    return temp;
}

// A utility function to create an empty lcs queue
lcsQueue* initialQueue() {
    lcsQueue* q = (lcsQueue*)malloc(sizeof(lcsQueue));
    q->front = q->rear = NULL;
    return q;
}

// The function to add a cell to lcs queue
void enQueue(lcsQueue* q, cell_t* cell) {
    cell->visited = 1;
    lcsNode* temp = newNode(cell);

    // If queue is empty, then new node is front and rear both
    if (q->front == NULL) {
        q->front = q->rear = temp;
        return;
    }

    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = q->rear->next;

    // Update parent
    temp->parent = q->front;
}

// Function to remove a cell from given lcs queue
void deQueue(lcsQueue* q, bool isFree) {
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return;

    // Store previous front and move front one node ahead
    lcsNode* temp = q->front;

    q->front = q->front->next;

    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;

    if (isFree)
        free(temp);
}
