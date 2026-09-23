#include "localization.h"

#include <QHash>
#include <QLocale>
#include <QRegularExpression>
#include <QSettings>

namespace {
const QHash<QString, QString> &englishTranslations()
{
    static const QHash<QString, QString> values{
        {"Benvenuto in NebiPad+", "Welcome to NebiPad+"},
        {"Scegli l'applicazione della suite.", "Choose a suite application."},
        {"LA SUITE NEBIPAD+", "THE NEBIPAD+ SUITE"},
        {"FILE RECENTI", "RECENT FILES"},
        {"I documenti aperti di recente compariranno qui.", "Recently opened documents will appear here."},
        {"I file recenti si aprono automaticamente nell'applicazione corretta.", "Recent files open automatically in the correct application."},
        {"Lingua", "Language"}, {"Italiano", "Italian"}, {"Inglese", "English"},
        {"Cambia lingua", "Change language"},
        {"Per applicare la nuova lingua a tutta la suite, NebiPad+ verrà riavviato. Salvare prima eventuali documenti aperti. Continuare?",
         "NebiPad+ will restart to apply the new language throughout the suite. Save any open documents first. Continue?"},
        {"&File", "&File"}, {"&Nuovo", "&New"}, {"Nuovo", "New"},
        {"&Apri...", "&Open..."}, {"Apri...", "Open..."}, {"Apri", "Open"},
        {"&Salva", "&Save"}, {"Salva", "Save"}, {"Salva &con nome...", "Save &As..."},
        {"Salva con nome...", "Save As..."}, {"Salva tutto", "Save All"},
        {"File recenti", "Recent Files"}, {"Nessun file recente", "No Recent Files"},
        {"&Chiudi scheda", "&Close Tab"}, {"Chiudi scheda", "Close Tab"},
        {"Chiudi tutte le schede", "Close All Tabs"}, {"Chiudi documento", "Close Document"},
        {"&Stampa...", "&Print..."}, {"Stampa...", "Print..."},
        {"Esporta in PDF...", "Export as PDF..."}, {"Esporta PDF...", "Export PDF..."},
        {"Esporta in PDF", "Export as PDF"}, {"PDF esportato: %1", "PDF exported: %1"},
        {"&Modifica", "&Edit"}, {"&Annulla", "&Undo"}, {"Annulla", "Cancel"},
        {"&Ripeti", "&Redo"}, {"Ripeti", "Redo"}, {"&Taglia", "Cu&t"}, {"Taglia", "Cut"},
        {"&Copia", "&Copy"}, {"Copia", "Copy"}, {"&Incolla", "&Paste"}, {"Incolla", "Paste"},
        {"Seleziona &tutto", "Select &All"}, {"&Ricerca", "&Search"},
        {"&Trova...", "&Find..."}, {"Trova...", "Find..."}, {"Trova", "Find"},
        {"&Sostituisci tutto...", "&Replace All..."}, {"Sostituisci tutto...", "Replace All..."},
        {"Sostituisci", "Replace"}, {"Trova successivo", "Find Next"}, {"Trova precedente", "Find Previous"},
        {"Testo da cercare:", "Text to find:"}, {"Testo non trovato", "Text not found"},
        {"Sostituisci con:", "Replace with:"}, {"Sostituzioni: %1", "Replacements: %1"},
        {"&Visualizza", "&View"}, {"Ingrandisci", "Zoom In"}, {"Riduci", "Zoom Out"},
        {"Ritorno a capo automatico", "Word Wrap"}, {"F&ormato", "F&ormat"},
        {"Grassetto", "Bold"}, {"Corsivo", "Italic"}, {"Sottolineato", "Underline"},
        {"Barrato", "Strikethrough"}, {"Colore del testo...", "Text Color..."},
        {"Colore testo...", "Text Color..."}, {"Colore del testo", "Text Color"},
        {"Allinea a sinistra", "Align Left"}, {"Centra", "Center"},
        {"Allinea a destra", "Align Right"}, {"Giustifica", "Justify"},
        {"Codifica", "Encoding"}, {"Converti in %1", "Convert to %1"},
        {"&Linguaggio", "&Language"}, {"Testo normale", "Normal Text"},
        {"&Configurazione", "&Settings"}, {"&Strumenti", "&Tools"}, {"Strumenti", "Tools"},
        {"&Finestra", "&Window"}, {"&Inserisci", "&Insert"}, {"&Layout", "&Layout"},
        {"&Riferimenti", "&References"}, {"&Linee", "&Lines"},
        {"Avvia registrazione macro", "Start Macro Recording"},
        {"Ferma registrazione macro", "Stop Macro Recording"},
        {"Riproduci macro registrata", "Play Recorded Macro"},
        {"Riproduci più volte...", "Run Multiple Times..."},
        {"Salva macro registrata...", "Save Recorded Macro..."},
        {"Gestisci macro...", "Manage Macros..."}, {"Macro registrate", "Recorded Macros"},
        {"Nessuna macro registrata", "No Recorded Macros"},
        {"Registra o seleziona prima una macro.", "Record or select a macro first."},
        {"Riproduci macro", "Run Macro"}, {"Numero di ripetizioni:", "Number of repetitions:"},
        {"Registrazione macro in corso...", "Recording macro..."},
        {"Macro registrata: %1 operazioni", "Macro recorded: %1 operations"},
        {"Registrazione macro vuota", "Empty macro recording"},
        {"Macro eseguita %1 volta/e", "Macro run %1 time(s)"},
        {"Salva macro", "Save Macro"}, {"Nome della macro:", "Macro name:"},
        {"Sostituisci macro", "Replace Macro"},
        {"Esiste già una macro chiamata “%1”. Sostituirla?", "A macro named “%1” already exists. Replace it?"},
        {"Macro “%1” salvata", "Macro “%1” saved"}, {"Gestisci macro", "Manage Macros"},
        {"Rinomina...", "Rename..."}, {"Elimina", "Delete"}, {"Rinomina macro", "Rename Macro"},
        {"Nuovo nome:", "New name:"},
        {"Generatore password casuali...", "Random Password Generator..."},
        {"Generatore password casuali", "Random Password Generator"},
        {"Generatore SHA-256 / SHA-512", "SHA-256 / SHA-512 Generator"},
        {"Generatore SHA-256 / SHA-512...", "SHA-256 / SHA-512 Generator..."},
        {"Registra voce", "Record Voice"}, {"Registra nota audio", "Record Audio Note"},
        {"Play / Pausa audio", "Play / Pause Audio"}, {"Stop audio", "Stop Audio"},
        {"Play / Pausa nota audio", "Play / Pause Audio Note"}, {"Stop nota audio", "Stop Audio Note"},
        {"Dettatura vocale", "Voice Dictation"}, {"Parla nel microfono", "Speak into the microphone"},
        {"Attendo il microfono...", "Waiting for the microphone..."},
        {"Trascrizione in corso...", "Transcribing..."},
        {"Dettatura attiva · F12 per terminare", "Dictation active · press F12 to stop"},
        {"Registrazione audio", "Audio Recording"}, {"Registrazione audio salvata.", "Audio recording saved."},
        {"Sola lettura", "Read Only"}, {"Fissa scheda", "Pin Tab"}, {"Fissa la scheda", "Pin Tab"},
        {"Rimuovi il pin", "Unpin"}, {"Rimuovi il pin dalla scheda", "Unpin Tab"},
        {"Documento impostato in sola lettura", "Document set to read only"},
        {"Documento nuovamente modificabile", "Document is editable again"},
        {"Scheda fissata", "Tab pinned"}, {"Pin rimosso dalla scheda", "Tab unpinned"},
        {"Salva file", "Save File"}, {"Apri documento", "Open Document"},
        {"Salva documento", "Save Document"}, {"Senza titolo", "Untitled"},
        {"Modifiche non salvate", "Unsaved Changes"}, {"Salvare le modifiche a %1?", "Save changes to %1?"},
        {"Salvataggio fallito", "Save Failed"}, {"Apertura fallita", "Open Failed"},
        {"Formato non valido", "Invalid Format"}, {"Errore di codifica", "Encoding Error"},
        {"Informazioni su %1", "About %1"}, {"Informazioni su NebiPad+", "About NebiPad+"},
        {"Informazioni su Qt", "About Qt"},
        {"Diagramma", "Diagram"}, {"Inserisci diagramma", "Insert Diagram"},
        {"Diagramma di flusso", "Flowchart"}, {"Mappa mentale", "Mind Map"},
        {"Organigramma", "Organization Chart"}, {"Albero decisionale", "Decision Tree"},
        {"Diagramma di processo", "Process Diagram"}, {"Diagramma di rete", "Network Diagram"},
        {"Diagramma a blocchi", "Block Diagram"}, {"Linea", "Line"},
        {"Linea continua", "Solid Line"}, {"Linea tratteggiata", "Dashed Line"},
        {"Freccia continua", "Solid Arrow"}, {"Freccia tratteggiata", "Dashed Arrow"},
        {"Continua", "Solid"}, {"Tratteggiata", "Dashed"}, {"Freccia", "Arrow"},
        {"Spessore", "Thickness"}, {"Spessore...", "Thickness..."}, {"Colore...", "Color..."},
        {"Colore linea...", "Line Color..."}, {"Colore forma...", "Shape Color..."},
        {"Elimina oggetto", "Delete Object"}, {"Porta avanti", "Bring Forward"},
        {"Porta indietro", "Send Backward"}, {"Aggiungi vertice", "Add Point"},
        {"Rimuovi ultimo vertice", "Remove Last Point"}, {"Tipo di linea", "Line Type"},
        {"Pagina", "Page"}, {"Formato pagina", "Page Size"}, {"Pagina orizzontale", "Landscape Page"},
        {"Mostra righelli", "Show Rulers"}, {"Mostra griglia", "Show Grid"},
        {"Seleziona e sposta", "Select and Move"}, {"Selezione attiva", "Selection Active"},
        {"Inserisci casella di testo", "Insert Text Box"}, {"Tipo di carattere", "Font"},
        {"Dimensione carattere", "Font Size"}, {"Aumenta dimensione testo", "Increase Text Size"},
        {"Diminuisci dimensione testo", "Decrease Text Size"}, {"Forme", "Shapes"},
        {"Diagrammi di flusso", "Flowcharts"}, {"Processo", "Process"}, {"Inizio / fine", "Start / End"},
        {"Decisione", "Decision"}, {"Documento", "Document"}, {"Connettore", "Connector"},
        {"Sottoprocesso", "Subprocess"}, {"Attesa", "Delay"}, {"Stampante", "Printer"},
        {"Telecamera IP", "IP Camera"}, {"Antenna", "Antenna"}, {"Intestazione...", "Header..."},
        {"Piè di pagina...", "Footer..."}, {"TESTI DELLA PAGINA", "PAGE TEXT"},
        {"Formattazione", "Formatting"}, {"Inserisci formula...", "Insert Formula..."},
        {"Espressione matematica (Unicode):", "Mathematical expression (Unicode):"}, {"Simboli", "Symbols"},
        {"PDF", "PDF"}, {"Strumenti PDF", "PDF Tools"}, {"Apri PDF...", "Open PDF..."},
        {"Crea PDF dal documento corrente...", "Create PDF from Current Document..."},
        {"Aggiungi testo al PDF...", "Add Text to PDF..."}, {"Aggiungi immagine al PDF...", "Add Image to PDF..."},
        {"Inserisci pagina A4...", "Insert A4 Page..."}, {"Elimina pagina...", "Delete Page..."},
        {"Ruota pagina di 90°...", "Rotate Page 90°..."}, {"Sposta pagina...", "Move Page..."},
        {"Unisci un altro PDF...", "Merge Another PDF..."}, {"Proprietà e metadati...", "Properties and Metadata..."},
        {"Titolo", "Title"}, {"Autore", "Author"}, {"Oggetto", "Subject"}, {"Parole chiave", "Keywords"},
        {"Paragrafo", "Paragraph"}, {"Normale", "Normal"}, {"Sottotitolo", "Subtitle"},
        {"Titolo 1", "Heading 1"}, {"Titolo 2", "Heading 2"}, {"Titolo 3", "Heading 3"},
        {"Stile del paragrafo", "Paragraph Style"}, {"Elenco puntato", "Bulleted List"},
        {"Elenco numerato", "Numbered List"}, {"Riduci rientro", "Decrease Indent"},
        {"Aumenta rientro", "Increase Indent"}, {"Evidenzia testo...", "Highlight Text..."},
        {"Apice", "Superscript"}, {"Pedice", "Subscript"}, {"Cancella formattazione", "Clear Formatting"},
        {"Elenchi", "Lists"}, {"Puntato", "Bulleted"}, {"Numerato", "Numbered"},
        {"Interlinea", "Line Spacing"}, {"Spazio dopo paragrafo", "Space After Paragraph"},
        {"Tabella...", "Table..."}, {"Inserisci tabella", "Insert Table"}, {"Righe:", "Rows:"},
        {"Colonne:", "Columns:"}, {"Grafico...", "Chart..."}, {"Tabella", "Table"},
        {"Aggiungi riga", "Add Row"}, {"Aggiungi colonna", "Add Column"},
        {"Elimina riga", "Delete Row"}, {"Elimina colonna", "Delete Column"},
        {"Unisci celle selezionate", "Merge Selected Cells"}, {"Dividi cella", "Split Cell"},
        {"Colore cella...", "Cell Color..."}, {"Bordi tabella...", "Table Borders..."},
        {"Cambia maiuscole/minuscole", "Change Case"}, {"TUTTO MAIUSCOLO", "UPPERCASE"},
        {"tutto minuscolo", "lowercase"}, {"Iniziali Maiuscole", "Title Case"},
        {"Direzione del paragrafo", "Paragraph Direction"}, {"Da sinistra a destra", "Left to Right"},
        {"Da destra a sinistra", "Right to Left"}, {"Immagine...", "Image..."},
        {"Inserisci immagine", "Insert Image"}, {"Video", "Video"}, {"Dal PC...", "From Computer..."},
        {"Da YouTube...", "From YouTube..."}, {"Indirizzo del video:", "Video URL:"},
        {"Ridimensiona e ruota...", "Resize and Rotate..."}, {"Ritaglia...", "Crop..."},
        {"Contorno testo", "Text Wrapping"}, {"In linea con il testo", "In Line with Text"},
        {"Immagine a sinistra, testo a destra", "Image Left, Text Right"},
        {"Immagine a destra, testo a sinistra", "Image Right, Text Left"},
        {"Collegamento...", "Link..."}, {"Indirizzo URL:", "URL:"},
        {"Linea orizzontale", "Horizontal Line"}, {"Data e ora", "Date and Time"},
        {"Interruzione di pagina", "Page Break"}, {"Inserisci indice dei titoli", "Insert Table of Contents"},
        {"Aggiorna indice dei titoli", "Update Table of Contents"}, {"Indice", "Table of Contents"},
        {"Inserisci segnalibro...", "Insert Bookmark..."}, {"Segnalibro", "Bookmark"},
        {"Vai al segnalibro", "Go to Bookmark"}, {"Nessun segnalibro", "No Bookmarks"},
        {"Inserisci nota finale...", "Insert Endnote..."}, {"Nota finale", "Endnote"},
        {"Note finali", "Endnotes"}, {"Margini", "Margins"}, {"Stretti", "Narrow"},
        {"Normali", "Normal"}, {"Largi", "Wide"}, {"Intestazione e piè di pagina", "Header and Footer"},
        {"Intestazione", "Header"}, {"Piè di pagina", "Footer"}, {"Numeri di pagina", "Page Numbers"},
        {"Inserimento", "Insert"}, {"Layout pagina", "Page Layout"},
        {"Inserisci grafico", "Insert Chart"}, {"Colonne", "Columns"}, {"Barre", "Bars"},
        {"Linee", "Lines"}, {"Torta", "Pie"}, {"Andamento", "Trend"},
        {"Mostra legenda", "Show Legend"}, {"Tipo", "Type"}, {"Tema colori", "Color Theme"},
        {"Categoria", "Category"}, {"Gennaio", "January"}, {"Febbraio", "February"},
        {"Marzo", "March"}, {"Aprile", "April"}, {"Aggiungi categoria", "Add Category"},
        {"Rimuovi categoria", "Remove Category"}, {"Aggiungi serie", "Add Series"},
        {"Rimuovi serie", "Remove Series"}, {"Riprova", "Retry"}, {"Apri su YouTube", "Open on YouTube"},
        {"Riproduci / Pausa", "Play / Pause"}, {"Volume", "Volume"},
        {"Scrivi l'intestazione", "Enter the header"}, {"Scrivi il piè di pagina", "Enter the footer"},
        {"Pagina 1", "Page 1"}, {"Sì", "Yes"}, {"No", "No"}, {"Risultato", "Result"},
        {"Inizio", "Start"}, {"Fine", "End"}, {"Idea centrale", "Central Idea"},
        {"Direzione", "Direction"}, {"Condizione?", "Condition?"}, {"Elaborazione", "Processing"},
        {"Dati", "Data"}, {"Nodo centrale", "Central Node"},
        {"orizzontale", "landscape"}, {"verticale", "portrait"}
        ,{"Registrazione in corso  ·  Stop per terminare", "Recording · press Stop to finish"}
        ,{"In ascolto  ·  F12 per terminare", "Listening · press F12 to stop"}
        ,{"Audio salvato nel documento", "Audio saved in the document"}
        ,{"Funzione nel documento", "Function in the document"}, {"Simbolo nel documento", "Symbol in the document"}
        ,{"Inserisci le credenziali OAuth di tipo Applicazione desktop create per NebiPad+ in Google Cloud Console.", "Enter the Desktop app OAuth credentials created for NebiPad+ in Google Cloud Console."}
        ,{"Impossibile avviare il collegamento OAuth locale.", "Unable to start the local OAuth connection."}
        ,{"Autorizzazione Google Drive nel browser…", "Authorizing Google Drive in the browser…"}
        ,{"Google Drive collegato.", "Google Drive connected."}, {"Google Drive collegato a %1.", "Google Drive connected to %1."}
        ,{"Collega prima un account Google Drive.", "Connect a Google Drive account first."}
        ,{"Salva prima il documento sul computer.", "Save the document on your computer first."}
        ,{"%1 salvato su Google Drive.", "%1 saved to Google Drive."}
        ,{"Trascina gli angoli per ridimensionare; usa il quadratino superiore per ruotare.", "Drag the corner handles to resize; use the top handle to rotate."}
        ,{"Trascina sull'immagine per scegliere l'area da conservare.", "Drag over the image to select the area to keep."}
        ,{"Registrazione audio in corso · premi Stop per terminare", "Audio recording · press Stop to finish"}
        ,{"Crea coppia di chiavi...", "Create Key Pair..."}
        ,{"Barra degli strumenti principale", "Main Toolbar"}, {"Barra di formattazione", "Formatting Toolbar"}
        ,{"Nome dalla A alla Z", "Name A to Z"}, {"Nome dalla Z alla A", "Name Z to A"}
        ,{"Percorso dalla A alla Z", "Path A to Z"}, {"Percorso dalla Z alla A", "Path Z to A"}
        ,{"Tipo dalla A alla Z", "Type A to Z"}, {"Tipo dalla Z alla A", "Type Z to A"}
        ,{"Dimensione crescente", "Size Ascending"}, {"Dimensione decrescente", "Size Descending"}
        ,{"Data modifica crescente", "Modified Date Ascending"}, {"Data modifica decrescente", "Modified Date Descending"}
        ,{"Sola lettura attiva · clicca per modificare", "Read only · click to edit"}
        ,{"Clicca per rendere il documento in sola lettura", "Click to make the document read only"}
        ,{"Rimuovi il pin prima di chiudere", "Unpin before closing"}
        ,{"Rimuovi il pin dalla scheda prima di chiuderla", "Unpin the tab before closing it"}
        ,{"Le schede fissate sono rimaste aperte", "Pinned tabs were left open"}
        ,{"Impossibile leggere il file come %1.", "Unable to read the file as %1."}
        ,{"Impossibile interpretare il file come %1.", "Unable to interpret the file as %1."}
        ,{"Termina la registrazione audio prima di avviare la dettatura.", "Stop audio recording before starting dictation."}
        ,{"Apri un documento prima di iniziare la dettatura.", "Open a document before starting dictation."}
        ,{"Scrivi o incolla il testo da elaborare", "Type or paste the text to process"}
        ,{"Seleziona un file", "Select a file"}, {"Numero di password:", "Number of passwords:"}
        ,{"Lettere minuscole (a-z)", "Lowercase letters (a-z)"}, {"Lettere maiuscole (A-Z)", "Uppercase letters (A-Z)"}
        ,{"Escludi caratteri ambigui (I, l, 1, O, 0, o, |)", "Exclude ambiguous characters (I, l, 1, O, 0, o, |)"}
        ,{"Le password generate compariranno qui", "Generated passwords will appear here"}
        ,{"Installa o configura GnuPG prima di creare una chiave.", "Install or configure GnuPG before creating a key."}
        ,{"Crea coppia di chiavi OpenPGP", "Create OpenPGP Key Pair"}
        ,{"La chiave privata rimane nel portachiavi GnuPG locale. Conserva la password in un luogo sicuro.", "The private key remains in the local GnuPG keyring. Store the password securely."}
        ,{"Inserisci un nome e un indirizzo e-mail valido.", "Enter a name and a valid email address."}
        ,{"Usa almeno 8 caratteri e verifica che le due password coincidano.", "Use at least 8 characters and make sure the passwords match."}
        ,{"Creazione della coppia di chiavi OpenPGP...", "Creating OpenPGP key pair..."}
        ,{"La coppia di chiavi per %1 è stata creata correttamente.", "The key pair for %1 was created successfully."}
        ,{"Installa o configura GnuPG prima di importare una chiave.", "Install or configure GnuPG before importing a key."}
        ,{"La chiave OpenPGP è stata importata correttamente.", "The OpenPGP key was imported successfully."}
        ,{"Apri un documento da crittografare.", "Open a document to encrypt."}
        ,{"Apri un documento OpenPGP da decrittografare.", "Open an OpenPGP document to decrypt."}
        ,{"Crittografia OpenPGP in corso...", "Encrypting with OpenPGP..."}
        ,{"Decrittografia OpenPGP in corso...", "Decrypting with OpenPGP..."}
        ,{"Inserisci la password della chiave privata:", "Enter the private key password:"}
        ,{"Documento decrittografato con la chiave privata disponibile", "Document decrypted with the available private key"}
        ,{"Dettatura inserita nel documento.", "Dictation inserted into the document."}
        ,{"Termina la dettatura prima di registrare l'audio.", "Stop dictation before recording audio."}
        ,{"L'operazione OpenPGP ha superato il tempo massimo.", "The OpenPGP operation timed out."}
        ,{"Errore durante la lettura: %1", "Read error: %1"}
        ,{"Seleziona almeno un gruppo di caratteri", "Select at least one character group"}
        ,{"Una registrazione è già in corso.", "A recording is already in progress."}
        ,{"Nessun microfono disponibile.", "No microphone available."}
        ,{"Formato audio del microfono non supportato.", "The microphone audio format is not supported."}
        ,{"Impossibile avviare il microfono.", "Unable to start the microphone."}
        ,{"Termina la registrazione prima di ascoltare.", "Stop recording before playback."}
        ,{"Seleziona un riferimento audio esistente nel testo.", "Select an existing audio reference in the text."}
        ,{"Alterna pagina verticale e orizzontale", "Switch between portrait and landscape"}
        ,{"Disegna una linea trascinando sul foglio", "Draw a line by dragging on the page"}
        ,{"Aggiungi un simbolo al foglio", "Add a symbol to the page"}
        ,{"Apri un documento Impagination prima di iniziare la dettatura.", "Open an Impagination document before starting dictation."}
        ,{"Dettatura inserita nel documento Impagination.", "Dictation inserted into the Impagination document."}
        ,{"Termina la dettatura prima di registrare una nota audio.", "Stop dictation before recording an audio note."}
        ,{"Rendi il documento in sola lettura", "Make the document read only"}
        ,{"Apri prima un documento PDF in Impagination.", "Open a PDF document in Impagination first."}
        ,{"PDF modificato e ricaricato", "PDF edited and reloaded"}
        ,{"PoDoFo non è disponibile in questa compilazione", "PoDoFo is unavailable in this build"}
        ,{"Applica Titolo 1, 2 o 3 prima di creare l'indice", "Apply Heading 1, 2, or 3 before creating the table of contents"}
        ,{"Esiste già un segnalibro con questo nome.", "A bookmark with this name already exists."}
        ,{"Seleziona un'immagine nel documento", "Select an image in the document"}
        ,{"Inserisci un indirizzo YouTube valido.", "Enter a valid YouTube URL."}
        ,{"Caricamento di YouTube...", "Loading YouTube..."}
        ,{"Questa compilazione di NebiPad+ non include il modulo Qt PDF.", "This build of NebiPad+ does not include the Qt PDF module."}
        ,{"Apertura PDF fallita", "Failed to open PDF"}, {"Il PDF è protetto da password.", "The PDF is password protected."}
        ,{"Il PDF non è leggibile o è danneggiato.", "The PDF cannot be read or is damaged."}
        ,{"Il diagramma deve essere un file MindNet JSON.", "The diagram must be a MindNet JSON file."}
        ,{"Trascina sul foglio per disegnare la linea. Esc per tornare alla selezione.", "Drag on the page to draw the line. Press Esc to return to selection."}
        ,{"Download di %1 da Google Drive…", "Downloading %1 from Google Drive…"}
        ,{"%1 aperto da Google Drive.", "%1 opened from Google Drive."}
        ,{"Caricamento di %1 su Google Drive…", "Uploading %1 to Google Drive…"}
        ,{"Scheda successiva", "Next Tab"}, {"Scheda precedente", "Previous Tab"}
        ,{"NebiPad+ 0.1.0\nEditor di testo per macOS, sviluppato con C++ e Qt 6.", "NebiPad+ 0.1.0\nText editor for macOS, built with C++ and Qt 6."}
        ,{"Il documento contiene caratteri non rappresentabili in %1.", "The document contains characters that cannot be represented in %1."}
        ,{"Il file non contiene il BOM richiesto. Usa «Converti in %1» per aggiungerlo.", "The file does not contain the required BOM. Use ‘Convert to %1’ to add it."}
        ,{"Il file selezionato non è un eseguibile GnuPG valido.", "The selected file is not a valid GnuPG executable."}
        ,{"Disattiva la sola lettura della scheda prima di crittografare il documento.", "Disable read only mode before encrypting the document."}
        ,{"Disattiva la sola lettura della scheda prima di decrittografare il documento.", "Disable read only mode before decrypting the document."}
        ,{"Installa GnuPG (Gpg4win su Windows, GnuPG su Linux o GPG Suite/Homebrew su macOS), oppure scegli l'eseguibile da Strumenti → OpenPGP.", "Install GnuPG (Gpg4win on Windows, GnuPG on Linux, or GPG Suite/Homebrew on macOS), or select the executable under Tools → OpenPGP."}
        ,{"Caricamento chiavi pubbliche...", "Loading public keys..."}
        ,{"Impossibile leggere le chiavi pubbliche.\n\n%1", "Unable to read public keys.\n\n%1"}
        ,{"Il portachiavi OpenPGP è vuoto. Crea una coppia di chiavi oppure importa la chiave pubblica del destinatario.", "The OpenPGP keyring is empty. Create a key pair or import the recipient's public key."}
        ,{"Scegli il destinatario. Solo chi possiede la chiave privata corrispondente potrà leggere il documento.", "Choose the recipient. Only the owner of the matching private key can read the document."}
        ,{"Il risultato sarà salvato nel documento come messaggio OpenPGP ASCII Armor.", "The result will be stored in the document as an ASCII armored OpenPGP message."}
        ,{"GnuPG non ha prodotto un messaggio valido.", "GnuPG did not produce a valid message."}
        ,{"Il documento attivo non contiene un messaggio OpenPGP ASCII Armor.", "The active document does not contain an ASCII armored OpenPGP message."}
        ,{"La chiave privata necessaria non è disponibile.", "The required private key is unavailable."}
        ,{"Cartella del documento non scrivibile: audio salvato in %1", "Document folder is not writable: audio saved in %1"}
        ,{"Impossibile registrare in %1. %2", "Unable to record to %1. %2"}
        ,{"Impossibile preparare il documento temporaneo per GnuPG: %1", "Unable to prepare the temporary document for GnuPG: %1"}
        ,{"Elaborati %1 byte di testo", "Processed %1 bytes of text"}
        ,{"Impossibile leggere il file: %1", "Unable to read the file: %1"}
        ,{"Impossibile creare la copia di sicurezza del PDF.", "Unable to create the PDF backup."}
        ,{"Impossibile sostituire il PDF modificato.", "Unable to replace the edited PDF."}
        ,{"YouTube caricato nel lettore integrato.", "YouTube loaded in the embedded player."}
        ,{"Il microfono si è interrotto durante la registrazione.", "The microphone stopped during recording."}
        ,{"La registrazione audio si è interrotta: impossibile scrivere il file.", "Audio recording stopped because the file could not be written."}
        ,{"Sono stati persi %1 blocchi audio durante la registrazione. La traccia può avere interruzioni.", "%1 audio blocks were lost during recording. The track may contain gaps."}
        ,{"Impossibile creare la cartella dei video: %1", "Unable to create the video folder: %1"}
        ,{"Impossibile copiare il video: %1", "Unable to copy the video: %1"}
        ,{"Reparto A", "Department A"}, {"Reparto B", "Department B"}, {"Reparto C", "Department C"}
        ,{"Nodo A", "Node A"}, {"Nodo B", "Node B"}, {"Nodo C", "Node C"}, {"Nodo D", "Node D"}
        ,{"Classe base", "Base Class"}, {"Classe A", "Class A"}, {"Classe B", "Class B"}
        ,{"Blocco A", "Block A"}, {"Blocco B", "Block B"}, {"Blocco C", "Block C"}
        ,{"Colore delle forme", "Shape Color"}, {"Colore dei collegamenti", "Connector Color"}
        ,{"%1   |   %2 %3   |   Trascina le forme; Ctrl+rotella per lo zoom", "%1   |   %2 %3   |   Drag shapes; Ctrl+wheel to zoom"}
    };
    return values;
}

QString translatedByRules(QString text)
{
    static const QList<QPair<QString, QString>> rules{
        {"Tutti i file", "All Files"}, {"File di testo", "Text Files"}, {"Documenti", "Documents"},
        {"Immagini", "Images"}, {"Eseguibili", "Executables"}, {"Chiavi OpenPGP", "OpenPGP Keys"},
        {"Impossibile", "Unable to"}, {"non riuscita", "failed"}, {"non riuscito", "failed"},
        {"non disponibile", "unavailable"}, {"non è disponibile", "is unavailable"},
        {"Seleziona", "Select"}, {"Inserisci", "Enter"}, {"Aggiungi", "Add"},
        {"Rimuovi", "Remove"}, {"Elimina", "Delete"}, {"Modifica", "Edit"},
        {"Apri", "Open"}, {"Salva", "Save"}, {"Crea", "Create"}, {"Copia", "Copy"},
        {"Caricamento", "Loading"}, {"Registrazione", "Recording"}, {"Decrittografia", "Decryption"},
        {"Crittografia", "Encryption"}, {"documento", "document"}, {"Documento", "Document"},
        {"chiave", "key"}, {"Chiave", "Key"}, {"pagina", "page"}, {"Pagina", "Page"},
        {"testo", "text"}, {"Testo", "Text"}, {"immagine", "image"}, {"Immagine", "Image"},
        {"file", "file"}, {"cartella", "folder"}, {"password", "password"},
        {"Nome", "Name"}, {"Numero", "Number"}, {"Lunghezza", "Length"},
        {"Larghezza", "Width"}, {"Altezza", "Height"}, {"Angolo", "Angle"},
        {"Colore", "Color"}, {"Dimensione", "Size"}, {"Spessore", "Thickness"},
        {"Righe", "Rows"}, {"Colonne", "Columns"}, {"Parole", "Words"},
        {"Caratteri", "Characters"}, {"Paragrafi", "Paragraphs"}, {"Linea", "Line"},
        {"Errore", "Error"}, {"Nessuna", "No"}, {"Nessun", "No"},
        {"prima", "first"}, {"corrente", "current"}, {"attivo", "active"},
        {"disponibile", "available"}, {"valido", "valid"}, {"della", "of the"},
        {"del", "of the"}, {"nel", "in the"}, {"sul", "on the"}, {"con", "with"},
        {"per", "for"}, {"da", "from"}, {"in corso", "in progress"}
    };
    for (const auto &rule : rules) {
        const QRegularExpression expression(QStringLiteral("(?<![\\p{L}\\p{N}_])%1(?![\\p{L}\\p{N}_])")
                                                .arg(QRegularExpression::escape(rule.first)));
        text.replace(expression, rule.second);
    }
    return text;
}
}

NebiTranslator::NebiTranslator(QObject *parent) : QTranslator(parent) {}

QString NebiTranslator::translate(const char *, const char *sourceText, const char *, int) const
{
    if (!sourceText) return {};
    const QString source = QString::fromUtf8(sourceText);
    const auto exact = englishTranslations().constFind(source);
    if (exact != englishTranslations().constEnd()) return exact.value();
    return translatedByRules(source);
}

QString suiteLanguage()
{
    const QString configured = QSettings().value("ui/language").toString();
    if (configured == "it" || configured == "en") return configured;
    return QLocale::system().language() == QLocale::Italian ? QStringLiteral("it") : QStringLiteral("en");
}

void setSuiteLanguage(const QString &language)
{
    QSettings().setValue("ui/language", language == "en" ? "en" : "it");
}
