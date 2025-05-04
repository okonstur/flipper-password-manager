#include "password_storage.h"
#include <furi_hal_usb_hid.h>
#include <toolbox/stream/file_stream.h>
#include <toolbox/stream/stream.h>

#define TAG "PasswordStorage"
#define PASSWORDS_FILE_DIRECTORY "/ext/passwords"
#define PASSWORDS_FILE_PATH PASSWORDS_FILE_DIRECTORY "/passwords.txt"

void password_list_init(PasswordList* list) {
    FURI_LOG_I(TAG, "Inicializace seznamu hesel");
    list->count = 0;
    memset(list->items, 0, sizeof(list->items));
}

bool password_list_load(PasswordList* list, const char* storage_path) {
    FURI_LOG_I(TAG, "Načítání hesel z %s", storage_path);
    
    Storage* storage = furi_record_open(RECORD_STORAGE);
    
    // Vytvoření adresáře, pokud neexistuje
    if(!storage_dir_exists(storage, PASSWORDS_FILE_DIRECTORY)) {
        FURI_LOG_I(TAG, "Vytváření adresáře %s", PASSWORDS_FILE_DIRECTORY);
        if(!storage_simply_mkdir(storage, PASSWORDS_FILE_DIRECTORY)) {
            FURI_LOG_E(TAG, "Nelze vytvořit adresář %s", PASSWORDS_FILE_DIRECTORY);
            furi_record_close(RECORD_STORAGE);
            return false;
        }
    }
    
    // Kontrola, zda soubor existuje
    if(!storage_file_exists(storage, storage_path)) {
        FURI_LOG_I(TAG, "Soubor %s neexistuje, vytvářím prázdný seznam", storage_path);
        furi_record_close(RECORD_STORAGE);
        return true; // Vrátíme true, protože prázdný seznam je validní stav
    }
    
    // Otevření souboru
    Stream* stream = file_stream_alloc(storage);
    if(!file_stream_open(stream, storage_path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FURI_LOG_E(TAG, "Nelze otevřít soubor %s", storage_path);
        stream_free(stream);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    
    // Načtení hesel
    password_list_init(list); // Reset seznamu
    
    char line[PASSWORD_MAX_LENGTH + NAME_MAX_LENGTH + 2]; // +2 pro oddělovač a \0
    while(stream_read_line(stream, line, sizeof(line))) {
        // Odstranění nového řádku
        char* newline = strchr(line, '\n');
        if(newline) *newline = '\0';
        
        // Rozdělení řádku na název a heslo
        char* separator = strchr(line, ':');
        if(!separator) continue; // Přeskočit neplatné řádky
        
        *separator = '\0'; // Rozdělit řetězec
        char* name = line;
        char* password = separator + 1;
        
        // Přidání hesla do seznamu
        if(!password_list_add(list, name, password)) {
            FURI_LOG_W(TAG, "Seznam hesel je plný, načteno %lu hesel", list->count);
            break;
        }
    }
    
    FURI_LOG_I(TAG, "Načteno %lu hesel", list->count);
    
    // Uzavření souboru
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
    
    return true;
}

bool password_list_save(PasswordList* list, const char* storage_path) {
    FURI_LOG_I(TAG, "Ukládání hesel do %s", storage_path);
    
    Storage* storage = furi_record_open(RECORD_STORAGE);
    
    // Vytvoření adresáře, pokud neexistuje
    if(!storage_dir_exists(storage, PASSWORDS_FILE_DIRECTORY)) {
        FURI_LOG_I(TAG, "Vytváření adresáře %s", PASSWORDS_FILE_DIRECTORY);
        if(!storage_simply_mkdir(storage, PASSWORDS_FILE_DIRECTORY)) {
            FURI_LOG_E(TAG, "Nelze vytvořit adresář %s", PASSWORDS_FILE_DIRECTORY);
            furi_record_close(RECORD_STORAGE);
            return false;
        }
    }
    
    // Otevření souboru
    Stream* stream = file_stream_alloc(storage);
    if(!file_stream_open(stream, storage_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FURI_LOG_E(TAG, "Nelze otevřít soubor %s pro zápis", storage_path);
        stream_free(stream);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    
    // Uložení hesel
    for(uint32_t i = 0; i < list->count; i++) {
        PasswordItem* item = &list->items[i];
        stream_write_format(stream, "%s:%s\n", item->name, item->password);
    }
    
    FURI_LOG_I(TAG, "Uloženo %lu hesel", list->count);
    
    // Uzavření souboru
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
    
    return true;
}

bool password_list_add(PasswordList* list, const char* name, const char* password) {
    if(list->count >= MAX_PASSWORDS) {
        FURI_LOG_E(TAG, "Seznam hesel je plný");
        return false;
    }
    
    PasswordItem* item = &list->items[list->count];
    
    // Kopírování názvu a hesla s ověřením délky
    strlcpy(item->name, name, NAME_MAX_LENGTH);
    strlcpy(item->password, password, PASSWORD_MAX_LENGTH);
    
    list->count++;
    
    return true;
}

bool password_list_remove(PasswordList* list, uint32_t index) {
    if(index >= list->count) {
        FURI_LOG_E(TAG, "Neplatný index %lu", index);
        return false;
    }
    
    // Posun všech položek za indexem o jednu pozici zpět
    for(uint32_t i = index; i < list->count - 1; i++) {
        memcpy(&list->items[i], &list->items[i + 1], sizeof(PasswordItem));
    }
    
    // Vymazání poslední položky
    memset(&list->items[list->count - 1], 0, sizeof(PasswordItem));
    
    list->count--;
    
    return true;
}

void password_send_as_keyboard(const char* password) {
    FURI_LOG_I(TAG, "Odesílání hesla jako klávesnice");
    
    // Kontrola, zda je USB HID připojen
    if(!furi_hal_usb_is_connected()) {
        FURI_LOG_E(TAG, "USB není připojeno");
        return;
    }
    
    // Odeslání hesla jako klávesnice
    for(size_t i = 0; i < strlen(password); i++) {
        char c = password[i];
        
        // Převod znaku na klávesový kód
        uint16_t keycode = 0;
        uint8_t mod = 0;
        
        if(c >= 'a' && c <= 'z') {
            keycode = HID_KEYBOARD_A + (c - 'a');
        } else if(c >= 'A' && c <= 'Z') {
            keycode = HID_KEYBOARD_A + (c - 'A');
            mod = KEY_MOD_LEFT_SHIFT;
        } else if(c >= '0' && c <= '9') {
            keycode = HID_KEYBOARD_0 + (c - '0');
        } else {
            // Speciální znaky
            switch(c) {
                case ' ':
                    keycode = HID_KEYBOARD_SPACEBAR;
                    break;
                case '!':
                    keycode = HID_KEYBOARD_1;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '@':
                    keycode = HID_KEYBOARD_2;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '#':
                    keycode = HID_KEYBOARD_3;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '$':
                    keycode = HID_KEYBOARD_4;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '%':
                    keycode = HID_KEYBOARD_5;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '^':
                    keycode = HID_KEYBOARD_6;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '&':
                    keycode = HID_KEYBOARD_7;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '*':
                    keycode = HID_KEYBOARD_8;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '(':
                    keycode = HID_KEYBOARD_9;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case ')':
                    keycode = HID_KEYBOARD_0;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '-':
                    keycode = HID_KEYBOARD_MINUS;
                    break;
                case '_':
                    keycode = HID_KEYBOARD_MINUS;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '=':
                    keycode = HID_KEYBOARD_EQUAL;
                    break;
                case '+':
                    keycode = HID_KEYBOARD_EQUAL;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '[':
                    keycode = HID_KEYBOARD_OPEN_BRACKET;
                    break;
                case '{':
                    keycode = HID_KEYBOARD_OPEN_BRACKET;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case ']':
                    keycode = HID_KEYBOARD_CLOSE_BRACKET;
                    break;
                case '}':
                    keycode = HID_KEYBOARD_CLOSE_BRACKET;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '\\':
                    keycode = HID_KEYBOARD_BACKSLASH;
                    break;
                case '|':
                    keycode = HID_KEYBOARD_BACKSLASH;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case ';':
                    keycode = HID_KEYBOARD_SEMICOLON;
                    break;
                case ':':
                    keycode = HID_KEYBOARD_SEMICOLON;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '\'':
                    keycode = HID_KEYBOARD_APOSTROPHE;
                    break;
                case '"':
                    keycode = HID_KEYBOARD_APOSTROPHE;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case ',':
                    keycode = HID_KEYBOARD_COMMA;
                    break;
                case '<':
                    keycode = HID_KEYBOARD_COMMA;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '.':
                    keycode = HID_KEYBOARD_DOT;
                    break;
                case '>':
                    keycode = HID_KEYBOARD_DOT;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                case '/':
                    keycode = HID_KEYBOARD_SLASH;
                    break;
                case '?':
                    keycode = HID_KEYBOARD_SLASH;
                    mod = KEY_MOD_LEFT_SHIFT;
                    break;
                default:
                    // Neznámý znak, přeskočíme
                    continue;
            }
        }
        
        // Odeslání klávesy
        furi_hal_usb_hid_keyboard_press(mod, keycode);
        furi_delay_ms(10);
        furi_hal_usb_hid_keyboard_release(keycode);
        furi_delay_ms(10);
    }
}
