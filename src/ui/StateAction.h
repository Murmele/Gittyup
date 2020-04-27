#ifndef STATEACTION_H
#define STATEACTION_H

#include <QAction>

/*!
 * \brief The StateAction class
 * Action which changes automatically the text when the state is changed
 * completely compatible with a normal action
 */
class StateAction: public QAction
{
    Q_OBJECT

public:
    StateAction(const QString activeText, const QString inactiveText, QObject *parent = nullptr);
    /*!
     * \brief isActive
     * Indicates if the action is active (true) or not (false)
     * \return
     */
    bool isActive();
signals:
    /*!
     * \brief triggered
     * Emitted when the action is triggered
     * \param checked. Defined like in QAction
     */
    void triggered(bool checked);
private:

    /*!
     * \brief setActive
     * set object to \p active and change the text
     * \param active
     */
    void setActive(bool active);
    /*!
     * \brief toggleActive
     * toggle active state
     */
    void toggleActive();

    /*!
     * State of the action
     */
    bool mActive{false};
    /*!
     * \brief mActiveText
     * Text shown when action is active
     */
    QString mActiveText;
    /*!
     * \brief mInactiveText
     * Text shown when action is inactive
     */
    QString mInactiveText;
};

#endif // STATEACTION_H
