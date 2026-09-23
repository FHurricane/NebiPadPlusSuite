#include "lspclient.h"

#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>

namespace {
struct ServerOption { const char *program; QStringList arguments; };

QStringList executableLocations(const QString &name)
{
    QStringList paths;
    const QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) paths << found;
#ifdef Q_OS_MACOS
    for (const QString &directory : {QStringLiteral("/opt/homebrew/bin"), QStringLiteral("/usr/local/bin")}) {
        const QString path = directory + '/' + name;
        if (QFileInfo(path).isExecutable() && !paths.contains(path)) paths << path;
    }
#endif
    return paths;
}
}

LspClient::LspClient(QObject *parent) : QObject(parent)
{
    connect(&process_, &QProcess::readyReadStandardOutput, this, &LspClient::readMessages);
    connect(&process_, &QProcess::readyReadStandardError, this, [this] { process_.readAllStandardError(); });
    connect(&process_, &QProcess::errorOccurred, this, [this] { callback_ = {}; });
}

LspClient::~LspClient()
{
    if (process_.state() != QProcess::NotRunning) {
        process_.kill();
        process_.waitForFinished(250);
    }
}

void LspClient::chooseServer(const QString &language)
{
    QVector<ServerOption> options;
    languageId_ = language.toLower();
    if (language == "C" || language == "C++" || language == "Objective-C") {
        options << ServerOption{"clangd", {"--background-index"}};
        languageId_ = language == "C++" ? "cpp" : language == "C" ? "c" : "objective-c";
    } else if (language == "Swift") {
        options << ServerOption{"sourcekit-lsp", {}};
    } else if (language == "Python") {
        options << ServerOption{"basedpyright-langserver", {"--stdio"}}
                << ServerOption{"pyright-langserver", {"--stdio"}}
                << ServerOption{"pylsp", {}}
                << ServerOption{"jedi-language-server", {}};
    } else if (language == "JavaScript" || language == "TypeScript") {
        options << ServerOption{"typescript-language-server", {"--stdio"}};
        languageId_ = language == "JavaScript" ? "javascript" : "typescript";
    } else if (language == "Rust") {
        options << ServerOption{"rust-analyzer", {}};
    } else if (language == "Go") {
        options << ServerOption{"gopls", {"serve"}};
    } else if (language == "Lua") {
        options << ServerOption{"lua-language-server", {}};
    } else if (language == "Bash" || language == "Shell" || language == "Zsh") {
        options << ServerOption{"bash-language-server", {"start"}};
        languageId_ = "shellscript";
    } else if (language == "HTML") {
        options << ServerOption{"vscode-html-language-server", {"--stdio"}};
        languageId_ = "html";
    } else if (language == "CSS") {
        options << ServerOption{"vscode-css-language-server", {"--stdio"}};
        languageId_ = "css";
    } else if (language == "JSON" || language == "JSON5") {
        options << ServerOption{"vscode-json-language-server", {"--stdio"}};
        languageId_ = "json";
    } else if (language == "YAML") {
        options << ServerOption{"yaml-language-server", {"--stdio"}};
    }
    for (const auto &option : options) {
        const auto paths = executableLocations(QString::fromLatin1(option.program));
        if (paths.isEmpty()) continue;
        program_ = paths.first();
        arguments_ = option.arguments;
        return;
    }
}

void LspClient::setDocument(const QString &language, const QString &path, const QString &text)
{
    callback_ = {};
    if (process_.state() != QProcess::NotRunning) {
        process_.kill();
        process_.waitForFinished(250);
    }
    buffer_.clear();
    program_.clear();
    arguments_.clear();
    initialized_ = false;
    pendingCompletion_ = false;
    version_ = 1;
    completionId_ = 0;
    syncKind_ = 1;
    text_ = text;
    chooseServer(language);
    if (program_.isEmpty()) return;
    QString documentPath = path;
    if (documentPath.isEmpty()) {
        const QString untitled = QDir::tempPath() + "/nebipad-" +
            QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString extension = languageId_;
        if (languageId_ == "objective-c") extension = "m";
        else if (languageId_ == "javascript") extension = "js";
        else if (languageId_ == "typescript") extension = "ts";
        else if (languageId_ == "shellscript") extension = "sh";
        else if (languageId_ == "python") extension = "py";
        else if (languageId_ == "rust") extension = "rs";
        else if (languageId_ == "go") extension = "go";
        else if (languageId_ == "lua") extension = "lua";
        else if (languageId_ == "yaml") extension = "yaml";
        extension.prepend('.');
        documentPath = untitled + extension;
    }
    uri_ = QUrl::fromLocalFile(QFileInfo(documentPath).absoluteFilePath()).toString();
    rootUri_ = path.isEmpty() ? QString() :
        QUrl::fromLocalFile(QFileInfo(path).absolutePath()).toString();
    connect(&process_, &QProcess::started, this, [this] {
        if (process_.state() != QProcess::Running || program_.isEmpty()) return;
        initializeId_ = nextId_++;
        QJsonObject capabilities{{"textDocument", QJsonObject{
            {"completion", QJsonObject{{"completionItem", QJsonObject{
                {"snippetSupport", true}, {"documentationFormat", QJsonArray{"plaintext", "markdown"}}}}}}}}};
        QJsonObject params{{"processId", QCoreApplication::applicationPid()},
                           {"rootUri", rootUri_.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(rootUri_)},
                           {"capabilities", capabilities},
                           {"clientInfo", QJsonObject{{"name", "NebiPad+"}, {"version", "0.1"}}}};
        send({{"jsonrpc", "2.0"}, {"id", initializeId_}, {"method", "initialize"}, {"params", params}});
    }, Qt::SingleShotConnection);
    process_.start(program_, arguments_);
}

void LspClient::send(const QJsonObject &message)
{
    if (process_.state() != QProcess::Running) return;
    const QByteArray json = QJsonDocument(message).toJson(QJsonDocument::Compact);
    process_.write("Content-Length: " + QByteArray::number(json.size()) + "\r\n\r\n" + json);
}

void LspClient::notify(const QString &method, const QJsonObject &params)
{
    send({{"jsonrpc", "2.0"}, {"method", method}, {"params", params}});
}

void LspClient::sync(const QString &text)
{
    if (!initialized_ || text == text_) return;
    QJsonObject change{{"text", text}};
    if (syncKind_ == 2) {
        const int lastNewline = text_.lastIndexOf('\n');
        const int lastLine = text_.count('\n');
        const int lastCharacter = text_.size() - lastNewline - 1;
        change.insert("range", QJsonObject{
            {"start", QJsonObject{{"line", 0}, {"character", 0}}},
            {"end", QJsonObject{{"line", lastLine}, {"character", lastCharacter}}}});
        change.insert("rangeLength", text_.size());
    }
    text_ = text;
    ++version_;
    notify("textDocument/didChange", {{"textDocument", QJsonObject{{"uri", uri_}, {"version", version_}}},
                                      {"contentChanges", QJsonArray{change}}});
}

void LspClient::requestCompletion(const QString &text, int line, int character,
                                  std::function<void(QVector<LspCompletion>)> callback)
{
    callback_ = std::move(callback);
    if (!initialized_) {
        text_ = text;
        pendingCompletion_ = true;
        pendingLine_ = line;
        pendingCharacter_ = character;
        return;
    }
    sync(text);
    completionId_ = nextId_++;
    send({{"jsonrpc", "2.0"}, {"id", completionId_}, {"method", "textDocument/completion"},
          {"params", QJsonObject{{"textDocument", QJsonObject{{"uri", uri_}}},
                                 {"position", QJsonObject{{"line", line}, {"character", character}}},
                                 {"context", QJsonObject{{"triggerKind", 1}}}}}});
}

void LspClient::readMessages()
{
    buffer_ += process_.readAllStandardOutput();
    while (true) {
        const int separator = buffer_.indexOf("\r\n\r\n");
        if (separator < 0) return;
        int length = -1;
        const auto headers = buffer_.left(separator).split('\n');
        for (const QByteArray &header : headers) {
            if (header.trimmed().toLower().startsWith("content-length:"))
                length = header.mid(header.indexOf(':') + 1).trimmed().toInt();
        }
        if (length < 0 || length > 16 * 1024 * 1024) { buffer_.clear(); return; }
        if (buffer_.size() < separator + 4 + length) return;
        const auto message = QJsonDocument::fromJson(buffer_.mid(separator + 4, length));
        buffer_.remove(0, separator + 4 + length);
        if (message.isObject()) handleMessage(message.object());
    }
}

void LspClient::handleMessage(const QJsonObject &message)
{
    const int id = message.value("id").toInt(-1);
    if (id == initializeId_ && message.contains("result")) {
        const QJsonObject capabilities = message.value("result").toObject().value("capabilities").toObject();
        const QJsonValue synchronization = capabilities.value("textDocumentSync");
        if (synchronization.isDouble()) syncKind_ = synchronization.toInt(1);
        else if (synchronization.isObject())
            syncKind_ = synchronization.toObject().value("change").toInt(1);
        initialized_ = true;
        notify("initialized", {});
        notify("textDocument/didOpen", {{"textDocument", QJsonObject{
            {"uri", uri_}, {"languageId", languageId_}, {"version", version_}, {"text", text_}}}});
        if (pendingCompletion_) {
            pendingCompletion_ = false;
            requestCompletion(text_, pendingLine_, pendingCharacter_, std::move(callback_));
        }
        return;
    }
    if (id == completionId_ && callback_) {
        QVector<LspCompletion> items;
        QJsonValue result = message.value("result");
        if (result.isObject()) result = result.toObject().value("items");
        for (const auto &value : result.toArray()) {
            const QJsonObject entry = value.toObject();
            const QString label = entry.value("label").toString();
            if (label.isEmpty()) continue;
            QString insertText = entry.value("insertText").toString(label);
            if (entry.value("textEdit").isObject())
                insertText = entry.value("textEdit").toObject().value("newText").toString(insertText);
            QString detail = entry.value("detail").toString();
            if (detail.isEmpty() && entry.value("labelDetails").isObject())
                detail = entry.value("labelDetails").toObject().value("description").toString();
            items << LspCompletion{label, insertText, detail, entry.value("kind").toInt(1),
                                   entry.value("insertTextFormat").toInt() == 2};
            if (items.size() >= 200) break;
        }
        auto callback = std::move(callback_);
        callback(items);
        return;
    }
    if (message.contains("method") && message.contains("id")) {
        QJsonValue result(QJsonValue::Null);
        if (message.value("method").toString() == "workspace/configuration") {
            QJsonArray values;
            for (const auto &ignored : message.value("params").toObject().value("items").toArray()) {
                Q_UNUSED(ignored)
                values.append(QJsonValue::Null);
            }
            result = values;
        }
        send({{"jsonrpc", "2.0"}, {"id", message.value("id")}, {"result", result}});
    }
}
