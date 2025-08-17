#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filtering.h"  // include your filtering logic

typedef struct {
    GtkWidget *genre;
    GtkWidget *language;
    GtkWidget *decade;
    GtkWidget *rating;
    GtkWidget *output_textview;
    GtkWidget *review_movie_entry;
    GtkWidget *review_display_textview;
    GtkWidget *review_textview;
} Widgets;   // widgets is the name of the structure

void recommend_movies(GtkButton *button, gpointer user_data) {
    Widgets *w = (Widgets *)user_data;

    const char *genre = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w->genre));
    const char *language = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w->language));
    const char *decade = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w->decade));
    const char *rating_str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w->rating));

    if (!genre || !language || !decade || !rating_str) return;

    float min_rating = atof(rating_str);
    const char *result = filter_movies_for_gui(genre, language, decade, min_rating);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->output_textview));
    gtk_text_buffer_set_text(buffer, result, -1);
}

void submit_review(GtkWidget *widget, gpointer user_data) {
    Widgets *w = (Widgets *)user_data;  // getting text from box
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->review_textview));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    size_t len = strlen(text); // removing extra lines
    if (len > 0 && text[len - 1] == '\n') text[len - 1] = '\0';

    char *comma = strchr(text, ','); // checking if review format if correct
    if (!comma || comma == text || comma[1] == '\0') {
        gtk_text_buffer_set_text(buffer, "Please enter the review in the given format.", -1);
        g_free(text);
        return;
    }

    FILE *file = fopen("Reviews.csv", "a"); // appending to the file
    if (!file) {
        gtk_text_buffer_set_text(buffer, "Error opening file to save review.", -1);
        g_free(text);
        return;
    }

    fprintf(file, "%s\n", text);
    fclose(file);

    gtk_text_buffer_set_text(buffer, "Review submitted successfully.", -1);
    g_free(text);
}

void display_reviews(const char* movie_name, GtkTextBuffer* buffer) {
    FILE* file = fopen("Reviews.csv", "r");
    if (!file) return;

    char line[4096]; // reading the lines in the file
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gboolean found = FALSE;

    while (fgets(line, sizeof(line), file)) { // handling multiline quotes
        while (strchr(line, '"') && (strrchr(line, '"') == strchr(line, '"'))) {
            char continuation[4096];
            if (fgets(continuation, sizeof(continuation), file)) {
                strcat(line, continuation);
            } else {
                break;
            }
        }

        char *comma = strchr(line, ','); // separates movie name from review
        if (!comma) continue;

        *comma = '\0';
        char *name = line;
        char *review = comma + 1;

        size_t len = strlen(review);
        if (len > 0 && review[len - 1] == '\n') review[len - 1] = '\0';
        if (review[0] == '"') review++; // removes quotes around the review, if any
        if (review[strlen(review) - 1] == '"') review[strlen(review) - 1] = '\0';

        if (strcmp(name, movie_name) == 0) {
            gtk_text_buffer_get_end_iter(buffer, &end); // if movie name matches, insert review to the gui
            gtk_text_buffer_insert(buffer, &end, review, -1);   // insert review text
            gtk_text_buffer_insert(buffer, &end, "\n", -1);   // add newline 
            found = TRUE;
        }
    }

    if (!found) {
        gtk_text_buffer_insert(buffer, &end, "(No reviews found)\n", -1);
    }

    fclose(file);
}

void show_reviews(GtkButton *button, gpointer user_data) {    
    Widgets *w = (Widgets *)user_data;

    const char *movie_name = gtk_entry_get_text(GTK_ENTRY(w->review_movie_entry)); // reading movie name entered by user
    if (!movie_name || strlen(movie_name) == 0) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->review_display_textview));
    gtk_text_buffer_set_text(buffer, "", -1); // clears previous reviews displayed
    display_reviews(movie_name, buffer);
}

int main(int argc, char *argv[]) {   
    gtk_init(&argc, &argv);   // initializes gtk, must be called before any function

    GtkWidget *window, *label, *grid, *button;   // creating window,label,grid,button 
    GtkWidget *combo_genre, *combo_language, *combo_decade, *combo_rating;    // creating dropdown box
    GtkWidget *review_label, *review_textview;   // review text box
    GtkWidget *output_textview, *output_scroll;  // to display output 
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Movie Recommendation and Review System");  // setting title
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 800);   // setting size
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);  // connects a signal to a callback function 

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(window), grid);

    // genre
    label = gtk_label_new("Genre:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);    // column,row,width,height
    combo_genre = gtk_combo_box_text_new();
    const char *genres[] = {"AI", "Action", "Adventure", "Animation", "Comedy", "Crime", "Drama", "Dystopian", "Epic", "Family",
                            "Fantasy", "History", "Horror", "Musical", "Mystery", "Romance", "Science Fiction", "Superhero",
                            "Survival", "Thriller", "Tragedy", "War"};
    for (int i = 0; i < 22; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_genre), genres[i]);    // adds each item to dropdown menu
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_genre), 0);    // default option is set as 0  (displays first option)
    gtk_grid_attach(GTK_GRID(grid), combo_genre, 1, 0, 1, 1);

    // language
    label = gtk_label_new("Language:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    combo_language = gtk_combo_box_text_new();
    const char *languages[] = {"English", "Hindi", "Korean", "Mandarin"};
    for (int i = 0; i < 4; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_language), languages[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_language), 0);
    gtk_grid_attach(GTK_GRID(grid), combo_language, 1, 1, 1, 1);

    // decade
    label = gtk_label_new("Decade:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
    combo_decade = gtk_combo_box_text_new();
    const char *decades[] = {"1990", "2000", "2010", "2020"};
    for (int i = 0; i < 4; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_decade), decades[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_decade), 0);
    gtk_grid_attach(GTK_GRID(grid), combo_decade, 1, 2, 1, 1);

    // minimum Rating
    label = gtk_label_new("Minimum Rating:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 3, 1, 1);
    combo_rating = gtk_combo_box_text_new();
    const char *ratings[] = {"5", "6", "7", "8", "9", "10"};
    for (int i = 0; i < 6; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_rating), ratings[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_rating), 0);
    gtk_grid_attach(GTK_GRID(grid), combo_rating, 1, 3, 1, 1);

    // recommend button
    button = gtk_button_new_with_label("Recommend");
    gtk_widget_set_size_request(button, 120, -1);   // width=120, height=-1 so it decides automatically
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 4, 2, 1);

    // recommended movies in textbox 
    label = gtk_label_new("Search Results:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 5, 1, 1);
    output_textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(output_textview), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output_textview), FALSE);
    gtk_widget_set_size_request(output_textview, 400, 150);
    gtk_grid_attach(GTK_GRID(grid), output_textview, 1, 5, 1, 1);

    // user review 
    review_label = gtk_label_new("Enter Review (Movie,Review):");
    gtk_grid_attach(GTK_GRID(grid), review_label, 0, 6, 1, 1);
    review_textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(review_textview), GTK_WRAP_WORD_CHAR);   
    GtkWidget *review_scroll = gtk_scrolled_window_new(NULL, NULL);   // text box with scroll
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(review_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);  // horizontal and vertical both are automatic
    gtk_widget_set_size_request(review_scroll, 350, 100);   // width,height 
    gtk_container_add(GTK_CONTAINER(review_scroll), review_textview);
    gtk_grid_attach(GTK_GRID(grid), review_scroll, 1, 6, 1, 1);

    // submit Review button
    GtkWidget *submit_review_button = gtk_button_new_with_label("Submit Review");
    gtk_widget_set_halign(submit_review_button, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), submit_review_button, 1, 7, 1, 1);

    // movie name entry for review lookup
    GtkWidget *review_movie_label = gtk_label_new("Movie Name for Review:");
    gtk_grid_attach(GTK_GRID(grid), review_movie_label, 0, 8, 1, 1);
    GtkWidget *review_movie_entry = gtk_entry_new();
    gtk_widget_set_size_request(review_movie_entry, 200, -1);
    gtk_grid_attach(GTK_GRID(grid), review_movie_entry, 1, 8, 1, 1);

    GtkWidget *show_reviews_button = gtk_button_new_with_label("Show Reviews");  // show review button
    gtk_widget_set_halign(show_reviews_button, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), show_reviews_button, 1, 9, 1, 1);

    GtkWidget *review_display_textview = gtk_text_view_new();    // text box to display reviews 
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(review_display_textview), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(review_display_textview), FALSE);
    gtk_widget_set_size_request(review_display_textview, 400, 150);
    gtk_grid_attach(GTK_GRID(grid), review_display_textview, 1, 10, 1, 1);

    // pack widgets for use in callback
    Widgets *widgets = g_malloc(sizeof(Widgets));
    widgets->genre = combo_genre;
    widgets->language = combo_language;
    widgets->decade = combo_decade;
    widgets->rating = combo_rating;
    widgets->output_textview = output_textview;
    widgets->review_movie_entry = review_movie_entry;
    widgets->review_display_textview = review_display_textview;
    widgets->review_textview = review_textview;

    // connect signals to buttons 
    g_signal_connect(button, "clicked", G_CALLBACK(recommend_movies), widgets);
    g_signal_connect(submit_review_button, "clicked", G_CALLBACK(submit_review),widgets);
    g_signal_connect(show_reviews_button, "clicked", G_CALLBACK(show_reviews), widgets);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}