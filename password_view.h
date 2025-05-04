#pragma once

#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/text_input.h>
#include <gui/modules/text_box.h>
#include <gui/modules/widget.h>
#include "password_storage.h"

// Definice scén
typedef enum {
    PasswordManagerSceneStart,
    PasswordManagerSceneMenu,
    PasswordManagerSceneList,
    PasswordManagerSceneView,
    PasswordManagerSceneAdd,
    PasswordManagerSceneAddName,
    PasswordManagerSceneAddPassword,
    PasswordManagerSceneDelete,
    PasswordManagerSceneConfirmDelete,
    PasswordManagerSceneAbout,
} PasswordManagerScene;

// Definice pohledů
typedef enum {
    PasswordManagerViewMenu,
    PasswordManagerViewList,
    PasswordManagerViewDialog,
    PasswordManagerViewTextInput,
    PasswordManagerViewTextBox,
    PasswordManagerViewWidget,
} PasswordManagerView;

// Struktura aplikace
typedef struct {
    // GUI
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    
    // Pohledy
    Submenu* submenu;
    DialogEx* dialog;
    TextInput* text_input;
    TextBox* text_box;
    Widget* widget;
    
    // Data
    PasswordList* password_list;
    uint32_t selected_index;
    char name_buffer[NAME_MAX_LENGTH];
    char password_buffer[PASSWORD_MAX_LENGTH];
    
    // Stav
    bool is_editing;
} PasswordManagerApp;

/**
 * @brief Alokuje a inicializuje aplikaci
 * 
 * @return PasswordManagerApp* Instance aplikace
 */
PasswordManagerApp* password_manager_app_alloc();

/**
 * @brief Uvolní aplikaci
 * 
 * @param app Instance aplikace
 */
void password_manager_app_free(PasswordManagerApp* app);

/**
 * @brief Spustí aplikaci
 * 
 * @param p Parametry (nepoužito)
 * @return int32_t Návratový kód
 */
int32_t password_manager_app(void* p);
