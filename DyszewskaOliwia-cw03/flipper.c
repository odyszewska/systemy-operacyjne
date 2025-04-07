#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>


void reverse_line(char *line) {
    int start = 0;
    int end = strlen(line) - 1;

    while (start < end) {
        char temp = line[start];
        line[start] = line[end];
        line[end] = temp;
        start++;
        end--;
    }
}

void process_file(const char *input_file, const char *output_file) {
    FILE *input = fopen(input_file, "r");
    FILE *output = fopen(output_file, "w");

    char line[1024];
    while (fgets(line, sizeof(line), input)) {
        reverse_line(line);
        fputs(line, output);
    }

    fclose(input);
    fclose(output);
}

int is_txt_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return (ext != NULL && strcmp(ext, ".txt") == 0);
}

void process_directory(const char *input_dir, const char *output_dir) {
    DIR *dir = opendir(input_dir);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char input_path[1024];
        snprintf(input_path, sizeof(input_path), "%s/%s", input_dir, entry->d_name);

        if (is_txt_file(entry->d_name)) {
            char output_path[1024];
            snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, entry->d_name);
            mkdir(output_dir, 0777);

            process_file(input_path, output_path);
        }
    }

    closedir(dir);
}



int main(int argc, char *argv[]) {
    process_directory(argv[1], argv[2]);
    return 0;
}
