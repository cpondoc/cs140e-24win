// convert the contents of stdin to their ASCII values (e.g., 
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(void) { 
    // Define buffer and character to read-in
    char s[1000000] = "";
    char ch;

    // Print the header of the code
    printf("char prog[] = {\n");

    // Loop and get all characters
    int i = 0;
    while ((ch = getchar()) != EOF) {
        // Print out the character, use the seed.c code
        printf("\t%d,%c", ch, (i+1)%8==0 ? '\n' : ' ');
        i = i + 1;

        // Store to a new character and print
        char charStr[2];
        sprintf(charStr, "%c", ch);
        strcat(s, charStr);
    }

    // Close the tab out and print the rest of the string
    s[i] = 0;
    printf("0 };\n");
    printf("%s", s);
	return 0;
}
