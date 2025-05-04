#pragma once

#include <furi.h>
#include <furi_hal.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>

#define PASSWORD_MAX_LENGTH 64
#define NAME_MAX_LENGTH 32
#define MAX_PASSWORDS 50

typedef struct {
    char name[NAME_MAX_LENGTH];
    char password[PASSWORD_MAX_LENGTH];
} PasswordItem;

typedef struct {
    PasswordItem items[MAX_PASSWORDS];
    uint32_t count;
} PasswordList;

/**
 * @brief Inicializuje seznam hesel
 * 
 * @param list Seznam hesel
 */
void password_list_init(PasswordList* list);

/**
 * @brief Načte hesla ze souboru
 * 
 * @param list Seznam hesel
 * @param storage_path Cesta k souboru
 * @return true Pokud se načtení podařilo
 * @return false Pokud se načtení nepodařilo
 */
bool password_list_load(PasswordList* list, const char* storage_path);

/**
 * @brief Uloží hesla do souboru
 * 
 * @param list Seznam hesel
 * @param storage_path Cesta k souboru
 * @return true Pokud se uložení podařilo
 * @return false Pokud se uložení nepodařilo
 */
bool password_list_save(PasswordList* list, const char* storage_path);

/**
 * @brief Přidá heslo do seznamu
 * 
 * @param list Seznam hesel
 * @param name Název hesla
 * @param password Heslo
 * @return true Pokud se přidání podařilo
 * @return false Pokud se přidání nepodařilo
 */
bool password_list_add(PasswordList* list, const char* name, const char* password);

/**
 * @brief Odstraní heslo ze seznamu
 * 
 * @param list Seznam hesel
 * @param index Index hesla
 * @return true Pokud se odstranění podařilo
 * @return false Pokud se odstranění nepodařilo
 */
bool password_list_remove(PasswordList* list, uint32_t index);

/**
 * @brief Odešle heslo jako klávesnici
 * 
 * @param password Heslo k odeslání
 */
void password_send_as_keyboard(const char* password);
