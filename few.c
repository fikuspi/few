#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>

#define MAX_LINES 10000
#define MAX_LEN 1024

typedef struct {
    char buf[MAX_LINES][MAX_LEN];
    int lines;
    int cur;
    char file[256];
    int modified;
} Editor;

Editor ed;
struct termios orig_term;

void clear_screen() {
    printf("\033[2J\033[H");
}

void show_help() {
    clear_screen();
    printf("\033[1mFew Editor - Simple Text Editor\033[0m\n\n");
    printf("\033[32m[Navigation]\033[0m\n");
    printf("n          Go to line n\n");
    printf(".          Show current line\n");
    printf("$          Go to last line\n");
    printf("+/-n       Move forward/backward n lines\n\n");
    
    printf("\033[32m[Viewing]\033[0m\n");
    printf("p          Print current line\n");
    printf("n,p        Print line n\n");
    printf("a,p        Print all lines\n");
    printf("a,n        Number all lines\n");
    printf("/text      Search forward\n");
    printf("?text      Search backward\n\n");
    
    printf("\033[32m[Editing]\033[0m\n");
    printf("a          Append after current line\n");
    printf("i          Insert before current line\n");
    printf("c          Change current line\n");
    printf("d          Delete current line\n");
    printf("n,d        Delete line n\n");
    printf("a,d        Delete all lines\n");
    printf("s/old/new  Substitute text\n\n");
    
    printf("\033[32m[File Operations]\033[0m\n");
    printf("w          Save\n");
    printf("w file     Save as\n");
    printf("q          Quit\n");
    printf("!cmd       Execute shell command\n");
    printf("h          Show this help\n");
}

void print_line(int n) {
    if (n < 1 || n > ed.lines) {
        printf("?\n");
        return;
    }
    printf("%d\t%s\n", n, ed.buf[n-1]);
    ed.cur = n;
}

void print_range(int start, int end) {
    if (start < 1) start = 1;
    if (end > ed.lines) end = ed.lines;
    for (int i = start; i <= end; i++)
        printf("%d\t%s\n", i, ed.buf[i-1]);
    ed.cur = end;
}

void save_file(const char *filename) {
    if (!filename[0] && !ed.file[0]) {
        printf("No filename specified\n");
        return;
    }
    if (filename[0]) strcpy(ed.file, filename);
    
    FILE *f = fopen(ed.file, "w");
    if (!f) {
        printf("Can't write file\n");
        return;
    }
    for (int i = 0; i < ed.lines; i++)
        fprintf(f, "%s\n", ed.buf[i]);
    fclose(f);
    ed.modified = 0;
    printf("Saved %d lines to %s\n", ed.lines, ed.file);
}

void edit_text(int mode) {
    printf("Enter text (end with single '.'):\n");
    char line[MAX_LEN];
    int start_line = mode == 'a' ? ed.cur+1 : ed.cur;
    
    while (1) {
        printf("%d> ", start_line);
        fgets(line, MAX_LEN, stdin);
        line[strcspn(line, "\n")] = 0;
        
        if (strcmp(line, ".") == 0) break;
        
        if (ed.lines >= MAX_LINES) {
            printf("Buffer full\n");
            break;
        }
        
        if (mode == 'a') {
            for (int i = ed.lines; i > ed.cur; i--)
                strcpy(ed.buf[i], ed.buf[i-1]);
            strcpy(ed.buf[ed.cur++], line);
            ed.lines++;
        } 
        else if (mode == 'i') {
            for (int i = ed.lines; i >= ed.cur; i--)
                strcpy(ed.buf[i], ed.buf[i-1]);
            strcpy(ed.buf[ed.cur-1], line);
            ed.lines++;
        } 
        else {
            strcpy(ed.buf[ed.cur-1], line);
        }
        ed.modified = 1;
        start_line++;
    }
}

void handle_command(const char *cmd) {
    if (isdigit(cmd[0])) {
        ed.cur = atoi(cmd);
        if (ed.cur < 1) ed.cur = 1;
        if (ed.cur > ed.lines) ed.cur = ed.lines;
        print_line(ed.cur);
    }
    else if (cmd[0] == '.') printf("%d\n", ed.cur);
    else if (cmd[0] == '$') print_line(ed.lines);
    else if (cmd[0] == '+' || cmd[0] == '-') {
        int n = cmd[1] ? atoi(cmd+1) : 1;
        ed.cur += (cmd[0] == '+' ? n : -n);
        if (ed.cur < 1) ed.cur = 1;
        if (ed.cur > ed.lines) ed.cur = ed.lines;
        print_line(ed.cur);
    }
    else if (cmd[0] == 'a' && cmd[1] == ',') {
        if (cmd[2] == 'p') print_range(1, ed.lines);
        else if (cmd[2] == 'n') {
            for (int i = 0; i < ed.lines; i++)
                printf("%d\t%s\n", i+1, ed.buf[i]);
            ed.cur = ed.lines;
        }
        else if (cmd[2] == 'd') {
            ed.lines = 0;
            ed.cur = 0;
            printf("All lines deleted\n");
        }
    }
    else if (cmd[0] == 'a') edit_text('a');
    else if (cmd[0] == 'i') edit_text('i');
    else if (cmd[0] == 'c') edit_text('c');
    else if (isdigit(cmd[0]) && cmd[1] == ',') {
        int n = atoi(cmd);
        if (cmd[2] == 'p') print_line(n);
        else if (cmd[2] == 'd') {
            if (n < 1 || n > ed.lines) {
                printf("?\n");
                return;
            }
            for (int i = n-1; i < ed.lines-1; i++)
                strcpy(ed.buf[i], ed.buf[i+1]);
            ed.lines--;
            ed.modified = 1;
            if (ed.cur >= n) ed.cur--;
            printf("Line %d deleted\n", n);
        }
    }
    else if (cmd[0] == 'd') {
        for (int i = ed.cur-1; i < ed.lines-1; i++)
            strcpy(ed.buf[i], ed.buf[i+1]);
        ed.lines--;
        ed.modified = 1;
        if (ed.cur > ed.lines) ed.cur = ed.lines;
        printf("Line %d deleted\n", ed.cur);
    }
    else if (cmd[0] == 'p') {
        if (cmd[1] == '\0') print_line(ed.cur);
        else if (isdigit(cmd[1])) print_line(atoi(cmd+1));
    }
    else if (cmd[0] == 'w') save_file(cmd+1);
    else if (cmd[0] == 'q') {
        if (ed.modified) {
            printf("Unsaved changes. Quit anyway? (y/n) ");
            if (getchar() != 'y') {
                while (getchar() != '\n');
                return;
            }
        }
        exit(0);
    }
    else if (cmd[0] == '!') system(cmd+1);
    else if (cmd[0] == 'h') show_help();
    else printf("?\n");
}

int main(int argc, char *argv[]) {
    memset(&ed, 0, sizeof(Editor));
    if (argc > 1) strcpy(ed.file, argv[1]);
    
    clear_screen();
    printf("Few Editor - type 'h' for help\n");
    
    if (ed.file[0]) {
        FILE *f = fopen(ed.file, "r");
        if (f) {
            while (fgets(ed.buf[ed.lines], MAX_LEN, f) && ed.lines < MAX_LINES) {
                ed.buf[ed.lines][strcspn(ed.buf[ed.lines], "\n")] = 0;
                ed.lines++;
            }
            fclose(f);
            ed.cur = ed.lines ? ed.lines : 1;
            printf("Loaded %d lines from %s\n", ed.lines, ed.file);
        }
    }
    
    char cmd[MAX_LEN];
    while (1) {
        printf("*");
        fflush(stdout);
        if (!fgets(cmd, MAX_LEN, stdin)) break;
        cmd[strcspn(cmd, "\n")] = 0;
        if (cmd[0]) handle_command(cmd);
    }
    
    return 0;
}
