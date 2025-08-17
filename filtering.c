#include <stdio.h>      // for FILE, fopen, fclose, fprintf, printf, snprintf
#include <stdlib.h>     // for atoi, atof, malloc, free
#include <string.h>     // for strcpy, strcat, strtok, strcmp, strcasecmp, strlen
#include <ctype.h>      // for isspace
#include <stdbool.h>    // for bool, true, false
#define Max 120         // max number of movies to load
#define Max_line 512    // max length of a line in the CSV file

typedef struct {
    char title[100];      
    char genre[200];      
    char language[50];    
    int year;             
    float rating;         
    int num_ratings;     
} Movie;

// remove trailing newline character from a string
void remove_line(char *str) {
    str[strcspn(str, "\n")] = 0;  // find newline and replace it with null terminator
}

int load_movies(Movie movies[], const char *filename) {
    FILE *file = fopen(filename, "r");   
    if (!file) {                         
        printf("Error: Could not open file '%s'. Please make sure it exists.\n", filename);
        return 0;                       
    }

    char line[Max_line];                 // buffer to store each line read
    int count = 0;                      // count how many movies loaded
    
    // skip header line 
    if (!fgets(line, sizeof(line), file)) {  
        fclose(file);                    
        printf("Error: Empty file or unable to read header.\n");
        return 0;
    }

    // read each line until EOF or max movies reached
    while (fgets(line, sizeof(line), file) && count < Max) {
        remove_line(line);               // remove trailing newline
        if (strlen(line) == 0) continue; // skip empty lines

        char *token = strtok(line, ","); // split line by commas
        if (!token) continue;             // if no token found, skip

        // copy first token as movie title
        strncpy(movies[count].title, token, sizeof(movies[count].title));

        token = strtok(NULL, ",");       // next token: genre
        if (!token) continue;
        strncpy(movies[count].genre, token, sizeof(movies[count].genre));

        token = strtok(NULL, ",");       // next token: language
        if (!token) continue;
        strncpy(movies[count].language, token, sizeof(movies[count].language));

        token = strtok(NULL, ",");       // next token: year
        if (!token) continue;
        movies[count].year = atoi(token); // convert string to int

        token = strtok(NULL, ",");       // next token: rating
        if (!token) continue;
        movies[count].rating = atof(token); // convert string to float

        token = strtok(NULL, ",");       // next token: num_ratings
        if (!token) continue;
        movies[count].num_ratings = atoi(token); // convert string to int

        count++;                        // increment movie count
    }

    fclose(file);                     
    return count;                     // return total movies loaded
}

// MAIN FILTERING FUNCTION for GUI
char* filter_movies_for_gui(const char *genre, const char *language, const char *decade_str, float min_rating) {
    static char result[10000];         // static buffer to store result string for return
    result[0] = '\0';                  // initialize buffer with empty string

    Movie movies[Max];                 // array to hold all loaded movies
    int count = load_movies(movies, "Movies.csv");  
    int decade = atoi(decade_str);    
    int decade_start = decade;        
    int decade_end = decade + 9;      

    Movie filtered[Max];              // array to hold filtered movies
    int fcount = 0;                   // count of filtered movies

    // loop over all loaded movies to filter
    for (int i = 0; i < count; i++) {
        if (movies[i].rating < min_rating) continue;
        if (strcasecmp(movies[i].language, language) != 0) continue;
        if (movies[i].year < decade_start || movies[i].year > decade_end) continue;

        // check if movie genre matches the filter genre
        char genre_copy[200];          // make a copy of genre string for tokenizing
        strcpy(genre_copy, movies[i].genre);

        char *token = strtok(genre_copy, "|");  // split genres by '|'
        int genre_match = 0;                     // flag to indicate if genre matched

        // check each genre token
        while (token != NULL) {
            while (isspace(*token)) token++;    // trim leading spaces
            if (strcasecmp(token, genre) == 0) { // case-insensitive compare to filter genre
                genre_match = 1;                 // genre matched
                break;
            }
            token = strtok(NULL, "|");           // get next genre token
        }
        // if genre matched, add movie to filtered list
        if (genre_match) {
            filtered[fcount++] = movies[i];
        }
    }
    
    // if filtered list is not empty, format the filtered movies into result string
    if (fcount > 0) {
        for (int i = 0; i < fcount; i++) {
            char line[256];
            snprintf(line, sizeof(line), "%s (%d) [%.1f]\n", filtered[i].title, filtered[i].year, filtered[i].rating);
            strcat(result, line);              // append formatted line to result string
        }
    } else {
        // if no filtered movies found, sort original movies by rating descending
        for (int i = 0; i < count - 1; i++) {
            for (int j = i + 1; j < count; j++) {
                if (movies[i].rating < movies[j].rating) {
                    Movie tmp = movies[i];     // swap movies[i] and movies[j]
                    movies[i] = movies[j];
                    movies[j] = tmp;
                }
            }
        }
        strcat(result, "No matching movies found. Showing top 5 rated movies:\n");   // add message to result string
        for (int i = 0; i < count && i < 5; i++) {      // append top 5 movies by rating to result string
            char line[256];
            snprintf(line, sizeof(line), "%s (%d) [%.1f]\n", movies[i].title, movies[i].year, movies[i].rating);
            strcat(result, line);
        }
    }
    return result;   // return pointer to static result string
}