// clang bundle.c -o bundle.out && ./bundle.out a/a.js a/y.js

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char base64[65] = "$0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

typedef struct {
    char * key;
    char * value;
} pair;

typedef struct {
    int length;
    pair ** pairs;
} map;

size_t sizeof_char_p = sizeof(char *);

map amap () {
    map a;
    a.length = 0;
    a.pairs = malloc(1);
    return a;
}

void delete (map * a, char * key) {
    for (int i = 0; i < a->length; i++) {
        if (strcmp(a->pairs[i]->key, key) == 0) {
            a->length--;
            for (int j = i; j < a->length; j++) {
                a->pairs[j] = a->pairs[j + 1];
            }
        }
    }
}

char * get (map * a, char * key) {
    for (int i = 0; i < a->length; i++) {
        if (strcmp(a->pairs[i]->key, key) == 0) {
            return a->pairs[i]->value;
        }
    }
    return 0;
}

void set (map * a, char * key, char * value) {
    delete(a, key);
    pair * p = malloc(sizeof_char_p * 2);
    p->key = key;
    p->value = value;
    a->pairs = realloc(a->pairs, (a->length + 1) * (sizeof_char_p * 2));
    a->pairs[a->length] = p;
    a->length++;
}

char * replaces (char * str, char * past, char * next) {
    int a = 0;
    int i = strlen(past);
    int j = strlen(next);
    char * p = str;
    while ((p = strstr(p, past))) {
        a++;
        p += i;
    }
    char * result = malloc(strlen(str) + a * (j - i) + 1);
    char * current = result;
    p = str;
    while ((p = strstr(p, past))) {
        int length = p - str;
        strncpy(current, str, length);
        current += length;
        strcpy(current, next);
        current += j;
        p += i;
        str = p;
    }
    strcpy(current, str);
    return result;
}

char * lstrip (char * str) {
    while (* str == '\t' || * str == ' ') {
        str++;
    }
    return str;
}

char * rstrip (char * str) {
    int i = -1;
    for (int j = 0; str[j] != 0; j++) {
        if (str[j] != '\t' && str[j] != ' ') {
            i = j;
        }
    }
    str[i + 1] = 0;
    return str;
}

int find_first_of (char * str, char * set, int start) {
    int length = strlen(str);
    if (start < length) {
        for (int i = start; i < length; i++) {
            if (strchr(set, str[i])) {
                return i;
            }
        }
    }
    return -1;
}

char * first_not_of (char * str) {
    while (* str && (* str == '\t' || * str == ' ')) {
        str++;
    }
    return str;
}

char ** splits (char * str, char * delimiter, int * length) {
    char * buffer = strdup(str);
    int i = 0;
    char * token = strtok(str, delimiter);
    while (token) {
        i++;
        token = strtok(0, delimiter);
    }
    char ** array = malloc(i * sizeof_char_p);
    if (i == 0) {
        array[i] = str;
        return array;
    }
    i = 0;
    token = strtok(buffer, delimiter);
    while (token) {
        array[i++] = token;
        token = strtok(0, delimiter);
    }
    * length = i;
    return array;
}

bool starts_with (char * str, char * prefix) {
    while (* prefix) {
        if (* prefix++ != * str++) {
            return false;
        }
    }
    return true;
}

char * substr (char * str, int start, int length) {
    char * sub = malloc(length * sizeof_char_p);
    memcpy(sub, str + start, length);
    sub[length] = 0;
    return sub;
}

char * substrs (char * str, int start) {
    return substr(str, start, strlen(str) - start);
}

char * resolve (char * f, char * file) {
    if (strncmp(f, "./", 2) == 0) {
        f += 2;
    }
    char i = f[0];
    if (i != '.' && i != '/') {
        int last = strrchr(file, '/') - file;
        char * str = malloc(last + strlen(f) + 1);
        strcpy(str, substr(file, 0, last));
        strcat(str, "/");
        strcat(str, f);
        f = str;
    } else if (strncmp(f, "../", 3) == 0) {
        while (strncmp(f, "../", 3) == 0) {
            f = substrs(f, 3);
            file = substr(file, 0, strrchr(file, '/') - file);
        }
        int last = strrchr(file, '/') - file;
        char * str = malloc(last + strlen(f) + 1);
        strcpy(str, substr(file, 0, last));
        strcat(str, "/");
        strcat(str, f);
        f = str;
    }
    if (strcmp(f + strlen(f) - 3, ".js") != 0) {
        strcat(f, ".js");
    }
    return f;
}

char * replace (char * text, char * past, char * next) {
    int a = 0;
    int i = strlen(past);
    int j = strlen(next);
    text = realloc(text, strlen(text) * j / i);
    char * ap = strstr(text, past);
    while (ap) {
        a = ap - text;
        if (strlen(text) < a + i + 1) {
            return text;
        }
        char previous = text[a - 1];
        if (strchr(base64, text[a + i]) || strchr(base64, previous) || strchr("\"'.", previous)) {
            a += i;
            ap = strstr(text + a, past);
            continue;
        }
        memmove(text + a + j, ap + i, strlen(text + a + i) + 1);
        memcpy(text + a, next, j);
        a += j;
        ap = strstr(text + a, past);
    }
    return text;
}

void parse (char * file, map * modules, map * texts) {
    FILE * f = fopen(file, "r");
    if (!f) {
        set(texts, file, "");
        return;
    }
    fseek(f, 0, 2);
    long length = ftell(f);
    rewind(f);
    char * text = malloc(length + 1);
    fread(text, 1, length, f);
    text[length] = 0;
    fclose(f);
    while (strstr(text, " \n")) {
        text = replaces(text, " \n", "\n");
    }
    while (strstr(text, "\n\n")) {
        text = replaces(text, "\n\n", "\n");
    }
    char * lines = text;
    bool remove = false;
    text = malloc(strlen(text) + 2);
    text[0] = 0;
    char * token = strtok(lines, "\n");
    while (token) {
        char * line = token;
        if (starts_with(lstrip(line), "//")) {
            token = strtok(0, "\n");
            continue;
        }
        char * ip = strstr(line, "/*");
        if (!remove && ip && !strstr(line, "//*")) {
            char * jp = strstr(line, "*/");
            if (jp) {
                char * str = malloc(strlen(line) + 1);
                strcpy(str, substr(line, 0, ip - line));
                strcat(str, " ");
                strcat(str, substrs(line, jp - line + 2));
                line = str;
            } else {
                line = substr(line, 0, ip - line);
                remove = true;
            }
        }
        if (remove) {
            if ((ip = strstr(line, "*/"))) {
                line = substrs(line, ip - line + 2);
                remove = false;
            } else {
                token = strtok(0, "\n");
                continue;
            }
        }
        line = rstrip(line);
        if (strlen(line) > 0) {
            strcat(text, line);
            strcat(text, "\n");
        }
        token = strtok(0, "\n");
    }
    char * texta = text;
    map files = amap();
    set(& files, file, "");
    char ** order = malloc(1);
    int order_length = 0;
    int order_size = 1;
    char * ip = strstr(text, "import ");
    while (ip) {
        int i = ip - text;
        char t = text[i - 1];
        if (i != 0 && t != '\t' && t != '\n' && t != ' ') {
            text = substrs(text, i + 6);
            ip = strstr(text, "import ");
            continue;
        }
        i += 6;
        while (text[i] == ' ') {
            i++;
        }
        text = substrs(text, i);
        ip = strstr(text, "from");
        i = ip - text;
        char * jp = strstr(text, "\"");
        char * kp = strstr(text, "'");
        char ** names = malloc(sizeof_char_p);
        int names_length = 0;
        int names_size = 1;
        if (ip && (!jp || i < jp - text) && (!kp || i < kp - text)) {
            while (i < strlen(text)) {
                char j = text[i - 1];
                char k = text[i + 4];
                if ((j == ' ' || j == '}') && (k == ' ' || k == '"' || k == '\'')) {
                    break;
                }
                i += 4;
                jp = substrs(text, i);
                ip = strstr(jp, "from");
                i += ip - jp;
            }
            int j = 0, k;
            char * t = substr(text, 0, i);
            while ((k = find_first_of(t, " ,{}", j)) != -1) {
                if (j < k) {
                    char * name = substr(t, j, k - j);
                    names_size += strlen(name) + 1;
                    names = realloc(names, (names_length + 1) * sizeof_char_p);
                    names[names_length++] = name;
                }
                j = k + 1;
            }
            if (j > strlen(t)) {
                char * name = substrs(t, j);
                names_size += strlen(name) + 1;
                names = realloc(names, (names_length + 1) * sizeof_char_p);
                names[names_length++] = name;
            }
            i += 5;
            while (text[i] == ' ') {
                i++;
            }
        } else {
            i = 0;
        }
        char f[2] = {text[i], 0};
        if (strcmp(f, "\"") == 0 || strcmp(f, "'") == 0) {
            text = substrs(text, i + 1);
            ip = strstr(text, f);
            char * f = resolve(substr(text, 0, ip - text), file);
            char * imported = get(& files, f);
            if (!imported) {
                imported = "";
                order = realloc(order, (order_length + 1) * sizeof_char_p);
                order[order_length++] = f;
                order_size += strlen(f) + 1;
            }
            char * imports = malloc(strlen(imported) + names_size);
            strcpy(imports, imported);
            for (int j = 0; j < names_length; j++) {
                strcat(imports, names[j]);
                strcat(imports, "\n");
            }
            set(& files, f, imports);
        }
        ip = strstr(text, "import ");
    }
    char * order_string = "";
    if (order_length > 0) {
        order_string = malloc(order_size);
        strcpy(order_string, order[0]);
        for (int i = 1; i < order_length; i++) {
            strcat(order_string, "\n");
            strcat(order_string, order[i]);
        }
    }
    set(modules, file, order_string);
    for (int i = 0; i < order_length; i++) {
        char * f = order[i];
        if (!get(texts, f)) {
            char * mods_string = get(modules, f);
            if (!mods_string) {
                return;
            } else {
                bool import = true;
                int length = 0;
                char ** mods = splits(mods_string, "\n", & length);
                for (int j = 0; j < length; j++) {
                    if (strcmp(file, mods[j]) == 0) {
                        import = false;
                        break;
                    }
                }
                if (import) {
                    return;
                }
            }
        }
    }
    char declares[7][9] = {"async", "class", "const", "default", "function", "let", "var"};
    char defines[6] = {'\n', ' ', '(', ',', '.', '['};
    strcpy(text, texta);
    ip = strstr(text, "export ");
    while (ip) {
        int i = ip - text;
        text = substrs(text, i + 7);
        for (int j = 0; j < 7; j++) {
            char * name = declares[j];
            ip = strstr(text, name);
            i = ip - text;
            if (ip && i < 3) {
                text = substrs(text, i + strlen(name));
            }
        }
        char * names = "";
        if ((ip = strstr(text, "\n"))) {
            names = substr(text, 0, ip - text);
        }
        i = 0;
        while (i < strlen(names) && names[i] == ' ') {
            i++;
        }
        char ** split = malloc(sizeof_char_p);
        int length = 0;
        if (i < strlen(names) && names[i] == '{') {
            names = substrs(names, i + 1);
            ip = strstr(names, "}");
            split = splits(substr(names, 0, ip - names), ",", & length);
        } else {
            ip = strstr(names, "(");
            i = ip - names;
            char * jp = strstr(names, "=");
            int j = jp - names;
            if (!jp || (i < j && ip)) {
                split[length++] = names;
            } else {
                while (jp) {
                    j = jp - names;
                    split = realloc(split, (length + 1) * sizeof_char_p);
                    split[length++] = substr(names, 0, j);
                    names = substrs(names, j);
                    jp = strstr(names, ",");
                    if (!jp) {
                        break;
                    }
                    names = substrs(names, jp - names);
                    jp = strstr(names, "=");
                }
            }
        }
        char * f = get(& files, file);
        for (i = 0; i < length; i++) {
            char * name = split[i];
            bool subs = true;
            while (strlen(name) && subs) {
                subs = false;
                char n = name[0];
                for (int j = 0; j < 6; j++) {
                    if (defines[j] == n) {
                        name = substrs(name, 1);
                        subs = true;
                        break;
                    }
                }
            }
            for (int j = 0; j < 6; j++) {
                char * jp = strstr(name, (char [2]) {defines[j], 0});
                if (jp) {
                    name = substr(name, 0, jp - name);
                }
            }
            char * str = malloc(strlen(f) + strlen(name) + 2);
            strcpy(str, f);
            strcat(str, name);
            strcat(str, "\n");
            f = str;
        }
        set(& files, file, f);
        ip = strstr(text, "export ");
    }
    text = texta;
    for (int i = 0, files_length = files.length; i < files_length; i++) {
        pair * p = files.pairs[i];
        char * f = p->key;
        int length = strlen(f);
        char * path = substr(f, 0, length - 3);
        for (int j = 0; j < length; j++) {
            if (!strchr(base64, path[j])) {
                path[j] = '_';
            }
        }
        length = 0;
        char ** names = splits(p->value, "\n", & length);
        for (int j = 0; j < length; j++) {
            char * name = names[j];
            char * next = malloc(strlen(name) + strlen(path) + 2);
            strcpy(next, name);
            strcat(next, "_");
            strcat(next, path);
            text = replace(text, name, next);
        }
    }
    lines = text;
    text = malloc(strlen(text) + 1);
    text[0] = 0;
    token = strtok(lines, "\n");
    while (token) {
        char * line = token;
        char * a = first_not_of(line);
        if (strncmp(a, "export default ", 15) == 0) {
            line = substrs(line, 15);
        } else if (strncmp(a, "export ", 7) == 0) {
            line = substrs(line, 7);
            a = first_not_of(line);
            if (* a == '{') {
                token = strtok(0, "\n");
                continue;
            }
        }
        if (strlen(line) > 0 && strncmp(a, "import ", 7) != 0) {
            strcat(text, line);
            strcat(text, "\n");
        }
        token = strtok(0, "\n");
    }
    set(texts, file, text);
}

void prepend (char *** array, int * length, char ** list, int list_length) {
    char ** result = malloc((* length + list_length) * sizeof_char_p);
    for (int i = 0; i < list_length; i++) {
        result[i] = list[i];
    }
    for (int i = 0; i < * length; i++) {
        result[i + list_length] = (* array)[i];
    }
    * array = result;
    * length += list_length;
}

void build (char * file, char * output) {
    char ** imported = malloc(1);
    int imported_length = 0;
    char ** imports = malloc(sizeof_char_p);
    int imports_length = 1;
    imports[0] = file;
    map modules = amap();
    map texts = amap();
    while (imports_length > 0) {
        file = imports[0];
        bool import = true;
        for (int i = 0; i < imported_length; i++) {
            if (strcmp(imported[i], file) == 0) {
                memmove(& imports[0], & imports[1], (imports_length - 1) * sizeof_char_p);
                imports_length--;
                import = false;
                break;
            }
        }
        if (import) {
            parse(file, & modules, & texts);
            char * mods_string = strdup(get(& modules, file));
            if (mods_string) {
                int length = 0;
                char ** mods = splits(mods_string, "\n", & length);
                if (length > 0) {
                    prepend(& imports, & imports_length, mods, length);
                }
            }
            if (get(& texts, file)) {
                imported = realloc(imported, (imported_length + 1) * sizeof_char_p);
                imported[imported_length++] = file;
            }
        }
    }
    int length = 1;
    for (int i = 0; i < imported_length; i++) {
        length += strlen(get(& texts, imported[i]));
    }
    char text[length];
    for (int i = 0; i < imported_length; i++) {
        strcat(text, get(& texts, imported[i]));
    }
    FILE * f = fopen(output, "w");
    if (f) {
        fputs(text, f);
        fclose(f);
    }
}

int main (int argc, char * argv[]) {
    build(argc > 1 ? argv[1] : "a/a.js", argc > 2 ? argv[2] : "a/y.js");
    return 0;
}