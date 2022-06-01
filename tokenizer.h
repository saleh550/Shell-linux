#pragma once

/* Operating Systems
 * Assignment 2 Code template
 * Semester 1 5782
 * Kinneret College
 * Code from UC Berkeley CS 162 course */

/* A struct that represents a list of words. */
struct tokens;

/* Turn a string into a list of words. */
struct tokens *tokenize(const char *line);

/* How many words are there? */
size_t tokens_get_length(struct tokens *tokens);

/* Get me the Nth word (zero-indexed) */
char *tokens_get_token(struct tokens *tokens, size_t n);

/* Free the memory */
void tokens_destroy(struct tokens *tokens);
