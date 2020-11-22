#ifndef EDITORHANDLER_H
#define EDITORHANDLER_H

#include <QSharedPointer>
#include <QObject>

class QWidget;
class Editor;
class TextEditor;
class QTimer;
/*!
 * \brief The EditorHandler class
 * Singletone to handle all editors used in the hunkwidgets.
 * The idea is to minimize the deleting and recreating of editors, because this takes
 * a lot of time.
 *
 * So this handler holds all editors and adds or deletes them if needed
 *
 * https://de.wikibooks.org/wiki/C%2B%2B-Programmierung:_Entwurfsmuster:_Singleton
 *
 * :-1: error: DWARF error: could not find variable specification at offset 298a7:
 * - does not find EditorHandler.cpp
 * solution: EditorHandler* EditorHandler::_instance = nullptr; must be in the source file, otherwise it is defined multiple times
 *
 * Class editor has a size about 3000 bytes. So it is not that important to delete all of them
 */
class EditorHandler: public QObject
{
    Q_OBJECT
public:
	static EditorHandler* instance() {
		static CGuard g;   // Speicherbereinigung
		if (!_instance)
			_instance = new EditorHandler();
		return _instance;
	}

    typedef struct EditorFree {
    public:
    public:
        bool occupied;
       Editor* editor; // TODO: use unique ptr here!
    }EditorFree_T;

    Editor* getEditor(QWidget* newParent);
    void releaseEditor(TextEditor *editor);
private:
    static EditorHandler* _instance;
    EditorHandler (); // {} // the constructor is private, so no other Instance can be created apart from this one.
    ~EditorHandler() {} // TODO: implement. Delete all editors
    EditorHandler ( const EditorHandler& ); // copy constructor
	class CGuard
	{
	public:
		~CGuard() {
			if(EditorHandler::_instance != nullptr) {
				delete EditorHandler::_instance;
				EditorHandler::_instance = nullptr;
			}
		}
	};
private slots:
    void deleteEditors();
    void editorDestroyed(QObject* e);
private:
    QTimer* mTimer;
    QVector<EditorFree> mEditors; // stores pointers to the editors
};
#endif // EDITORHANDLER_H
