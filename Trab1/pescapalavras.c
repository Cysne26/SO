#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_WORDS 100
#define MAX_WORD_LEN 100

// Estrutura para guardar os resultados de cada palavra
typedef struct {
    char word[MAX_WORD_LEN];
    bool found;
    int start_row;
    int start_col;
    char direction[20];
} WordData;

// Variáveis Globais (Compartilhadas entre as threads)
char **grid;
int rows, cols;
WordData words[MAX_WORDS];
int word_count = 0;
pthread_mutex_t grid_mutex; // Mutex para proteger a escrita na matriz

// Vetores de direção: (Linha, Coluna)
// 0: Cima-Esq | 1: Cima | 2: Cima-Dir | 3: Esq | 4: Dir | 5: Baixo-Esq | 6: Baixo | 7: Baixo-Dir
int d_row[] = {-1, -1, -1,  0, 0,  1, 1, 1};
int d_col[] = {-1,  0,  1, -1, 1, -1, 0, 1};
const char *dir_names[] = {
    "cima/esquerda", "cima", "direita/cima", 
    "esquerda", "direita", 
    "baixo/esquerda", "baixo", "baixo/direita"
};

// Função executada por cada thread
void *search_word(void *arg) {
    int word_idx = *((int *)arg);
    free(arg); // Libera o ponteiro alocado na main
    
    char *target = words[word_idx].word;
    int len = strlen(target);
    
    // Varre a matriz inteira
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            
            // Se a primeira letra bater
            if (toupper(grid[r][c]) == toupper(target[0])) {
                
                // Tenta nas 8 direções
                for (int d = 0; d < 8; d++) {
                    int k, curr_r = r, curr_c = c;
                    
                    for (k = 1; k < len; k++) {
                        curr_r += d_row[d];
                        curr_c += d_col[d];
                        
                        // Verifica limites da matriz
                        if (curr_r < 0 || curr_r >= rows || curr_c < 0 || curr_c >= cols) break;
                        // Verifica se a letra bate (usa toupper para ignorar se outra thread já capitalizou)
                        if (toupper(grid[curr_r][curr_c]) != toupper(target[k])) break;
                    }
                    
                    // Se k chegou ao tamanho da palavra
                    if (k == len) {
                        // INÍCIO DA ZONA CRÍTICA - Protegendo com Mutex
                        pthread_mutex_lock(&grid_mutex);
                        
                        words[word_idx].found = true;
                        // +1 para usar índice começando em 1 [cite: 52, 53]
                        words[word_idx].start_row = r + 1; 
                        words[word_idx].start_col = c + 1;
                        strcpy(words[word_idx].direction, dir_names[d]);
                        
                        // Capitaliza as letras na matriz
                        curr_r = r;
                        curr_c = c;
                        for (int i = 0; i < len; i++) {
                            grid[curr_r][curr_c] = toupper(grid[curr_r][curr_c]);
                            curr_r += d_row[d];
                            curr_c += d_col[d];
                        }
                        
                        pthread_mutex_unlock(&grid_mutex);
                        // FIM DA ZONA CRÍTICA
                        
                        pthread_exit(NULL); // Sai da thread pois já achou
                    }
                }
            }
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // Tratamento básico de erro
    if (argc != 2) {
        printf("Uso: %s <arquivo_de_entrada.txt>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    // Leitura das dimensões
    if (fscanf(file, "%d %d", &rows, &cols) != 2) {
        printf("Erro ao ler dimensoes.\n");
        fclose(file);
        return 1;
    }

    // Alocação dinâmica da matriz
    grid = malloc(rows * sizeof(char *));
    for (int i = 0; i < rows; i++) {
        grid[i] = malloc((cols + 1) * sizeof(char));
        fscanf(file, "%s", grid[i]);
    }

    // Leitura das palavras
    while (fscanf(file, "%s", words[word_count].word) == 1) {
        words[word_count].found = false;
        word_count++;
        if (word_count >= MAX_WORDS) break;
    }
    fclose(file);

    // Inicializa o Mutex 
    pthread_mutex_init(&grid_mutex, NULL);
    pthread_t threads[MAX_WORDS];

    // Dispara as threads
    for (int i = 0; i < word_count; i++) {
        int *arg = malloc(sizeof(int));
        *arg = i;
        if (pthread_create(&threads[i], NULL, search_word, arg) != 0) {
            perror("Erro ao criar thread");
            return 1;
        }
    }

    // Aguarda todas as threads terminarem
    for (int i = 0; i < word_count; i++) {
        pthread_join(threads[i], NULL);
    }

    // Imprime a matriz resultante
    for (int i = 0; i < rows; i++) {
        printf("%s\n", grid[i]);
    }

    printf("\n");

    // Imprime o relatorio das palavras
    for (int i = 0; i < word_count; i++) {
        if (words[i].found) {
            printf("%s (%d,%d): %s\n", words[i].word, words[i].start_row, words[i].start_col, words[i].direction);
        } else {
            printf("%s: nao encontrada\n", words[i].word);
        }
    }

    // Limpeza de memória e mutex
    for (int i = 0; i < rows; i++) {
        free(grid[i]);
    }
    free(grid); 
    pthread_mutex_destroy(&grid_mutex);

    return 0;
}

//cd Trab1
//gcc -o pescapalavras pescapalavras.c -pthread -Wall
//./pescapalavras cacapalavras.txt