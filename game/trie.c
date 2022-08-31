#include "trie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Source : https://www.journaldev.com/36507/trie-data-structure-in-c-plus-plus
// Since there are 26 english letters, we need
// 26 children per node
#define N 26

typedef struct TrieNode TrieNode;

struct TrieNode{
  // The Trie Node Structure
  // Each node has N children, starting from the root
  // and a flag to check if it's a leaf node
  char data; // Storing for printing purposes only
  TrieNode *children[N];
  int is_leaf;
};

TrieNode *make_trienode(char data){
  // Allocate memory for a TrieNode
  TrieNode *node = (TrieNode *)calloc(1, sizeof(TrieNode));
  for (int i = 0; i < N; i++){
    node->children[i] = NULL;
  }
  node->is_leaf = 0;
  node->data = data;
  return node;
}

void free_trienode(TrieNode *node){
  // Free the trienode sequence
  for (int i = 0; i < N; i++){
    if (node->children[i] != NULL){
      free_trienode(node->children[i]);
    }
    else{
      continue;
    }
  }
  free(node);
}

TrieNode *insert_trie(TrieNode *root, char *word){
  // Inserts the word onto the Trie
  // ASSUMPTION: The word only has lower case characters
  TrieNode *temp = root;

  for (int i = 0; word[i] != '\0'; i++){
    // Get the relative position in the alphabet list
    int idx = (int)word[i] - 'a';
    if (temp->children[idx] == NULL){
      // If the corresponding child doesn't exist,
      // simply create that child!
      temp->children[idx] = make_trienode(word[i]);
    }
    else{
      // Do nothing. The node already exists
    }
    // Go down a level, to the child referenced by idx
    // since we have a prefix match
    temp = temp->children[idx];
  }
  // At the end of the word, mark this node as the leaf node
  temp->is_leaf = 1;
  return root;
}

int search_trie(TrieNode *root, char *word){
  // Searches for word in the Trie
  TrieNode *temp = root;

  for (int i = 0; word[i] != '\0'; i++){
    int position = word[i] - 'a';
    if (temp->children[position] == NULL){
      return 0;
    }
    temp = temp->children[position];
  }
  if (temp != NULL && temp->is_leaf == 1){
    return 1;
  }
  return 0;
}

int check_divergence(TrieNode *root, char *word){
  // Checks if there is branching at the last character of word
  // and returns the largest position in the word where branching occurs
  TrieNode *temp = root;
  int len = strlen(word);
  if (len == 0){
    return 0;
  }
  // We will return the largest index where branching occurs
  int last_index = 0;
  for (int i = 0; i < len; i++){
    int position = word[i] - 'a';
    if (temp->children[position]){
      // If a child exists at that position
      // we will check if there exists any other child
      // so that branching occurs
      for (int j = 0; j < N; j++){
        if (j != position && temp->children[j]){
          // We've found another child! This is a branch.
          // Update the branch position
          last_index = i + 1;
          break;
        }
      }
      // Go to the next child in the sequence
      temp = temp->children[position];
    }
  }
  return last_index;
}

char *find_longest_prefix(TrieNode *root, char *word){
  // Finds the longest common prefix substring of word
  // in the Trie
  if (!word || word[0] == '\0'){
    return NULL;
  }
  // Length of the longest prefix
  int len = strlen(word);

  // We initially set the longest prefix as the word itself,
  // and try to back-tracking from the deepst position to
  // a point of divergence, if it exists
  char *longest_prefix = (char *)calloc(len + 1, sizeof(char));
  for (int i = 0; word[i] != '\0'; i++){
    longest_prefix[i] = word[i];
  }
  longest_prefix[len] = '\0';

  // If there is no branching from the root, this
  // means that we're matching the original string!
  // This is not what we want!
  int branch_idx = check_divergence(root, longest_prefix) - 1;
  if (branch_idx >= 0){
    // There is branching, We must update the position
    // to the longest match and update the longest prefix
    // by the branch index length
    longest_prefix[branch_idx] = '\0';
    longest_prefix = (char *)realloc(longest_prefix, (branch_idx + 1) * sizeof(char));
  }

  return longest_prefix;
}

int is_leaf_node(TrieNode *root, char *word){
  // Checks if the prefix match of word and root
  // is a leaf node
  TrieNode *temp = root;
  for (int i = 0; word[i]; i++){
    int position = (int)word[i] - 'a';
    if (temp->children[position]){
      temp = temp->children[position];
    }
  }
  return temp->is_leaf;
}

TrieNode *delete_trie(TrieNode *root, char *word){
  // Will try to delete the word sequence from the Trie only it
  // ends up in a leaf node
  if (!root){
    return NULL;
  }
  if (!word || word[0] == '\0'){
    return root;
  }
  // If the node corresponding to the match is not a leaf node,
  // we stop
  
  if (!is_leaf_node(root, word)){
    return root;
  }
  TrieNode *temp = root;
  // Find the longest prefix string that is not the current word
  char *longest_prefix = find_longest_prefix(root, word);
  if (longest_prefix[0] == '\0'){
    free(longest_prefix);
    return root;
  }
  // Keep track of position in the Trie
  int i;
  for (i = 0; longest_prefix[i] != '\0'; i++){
    int position = (int)longest_prefix[i] - 'a';
    if (temp->children[position] != NULL){
      // Keep moving to the deepest node in the common prefix
      temp = temp->children[position];
    }
    else{
      // There is no such node. Simply return.
      free(longest_prefix);
      return root;
    }
  }
  // Now, we have reached the deepest common node between
  // the two strings. We need to delete the sequence
  // corresponding to word
  int len = strlen(word);
  for (; i < len; i++){
    int position = (int)word[i] - 'a';
    if (temp->children[position]){
      // Delete the remaining sequence
      TrieNode *rm_node = temp->children[position];
      temp->children[position] = NULL;
      free_trienode(rm_node);
    }
  }
  free(longest_prefix);
  return root;
}

void print_trie(TrieNode *root){
  // Prints the nodes of the trie
  if (!root){
    return;
  }
  TrieNode *temp = root;
  printf("%c -> ", temp->data);
  for (int i = 0; i < N; i++){
    print_trie(temp->children[i]);
  }
}

int print_search(TrieNode *root, char *word){
  if (search_trie(root, word) == 0){
    //doesn't exist
    return 1;
  }
  else{
    //exists
    delete_trie(root, word);
    return 0;
  }
}

char *trimwhitespace(char *str){
  char *end;

  // Trim leading space
  while (isspace((unsigned char)*str)){
    str++;
  }
  if (*str == 0){ // All spaces?
    return str;
  }
  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end)){
    end--;
  }
  // Write new null terminator character
  end[1] = '\0';

  return str;
}

int trie(char *fileName, char *guess){
  // Driver program for the Trie Data Structure Operations
  TrieNode *root = make_trienode('\0');
  FILE *file = fopen(fileName, "r"); /* should check the result */
  char line[256];
  while (fgets(line, sizeof(line), file)){
    trimwhitespace(line);
    root = insert_trie(root, line);
  }
  fclose(file);
  int rightOrWrong = print_search(root, guess);
  delete_trie(root, guess);
  free_trienode(root);
  return rightOrWrong;
}
