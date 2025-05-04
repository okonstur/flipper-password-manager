# Password Manager pro Flipper Zero

Jednoduchý správce hesel pro Flipper Zero, který umožňuje ukládat, procházet a odesílat hesla.

## Funkce

- Ukládání hesel v textovém souboru na SD kartě
- Procházení uložených hesel
- Zobrazení hesla
- Odeslání hesla jako klávesnice
- Přidání nového hesla
- Smazání hesla

## Ovládání

### Hlavní obrazovka
- **OK**: Zobrazit seznam hesel
- **Vpravo**: Zobrazit nápovědu
- **Zpět**: Ukončit aplikaci

### Seznam hesel
- **Nahoru/Dolů**: Procházet seznam hesel
- **OK**: Zobrazit vybrané heslo
- **Dlouhý stisk OK**: Přidat nové heslo
- **Zpět**: Návrat na hlavní obrazovku

### Zobrazení hesla
- **OK**: Odeslat heslo jako klávesnici
- **Dlouhý stisk OK**: Smazat heslo
- **Zpět**: Návrat na seznam hesel

### Přidání hesla
- **Dlouhý stisk OK**: Přepnout mezi názvem a heslem
- **Dlouhý stisk Zpět**: Uložit heslo
- **Zpět**: Zrušit přidání hesla

## Formát souboru

Hesla jsou uložena v textovém souboru `/ext/passwords/passwords.txt` ve formátu:

```
název:heslo
```

Každé heslo je na samostatném řádku.

## Kompilace

Pro kompilaci aplikace je potřeba mít nainstalovaný Flipper Zero SDK. Poté stačí spustit:

```
./fbt fap_password_manager
```

## Autor

Vytvořeno pomocí Augment Agent
