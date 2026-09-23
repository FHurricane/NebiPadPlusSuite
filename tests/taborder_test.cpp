#include <QApplication>
#include <QLabel>
#include <QTabBar>
#include <QTabWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTabWidget tabs;
    auto *a = new QLabel("A");
    auto *b = new QLabel("B");
    auto *c = new QLabel("C");
    tabs.addTab(a, "A");
    tabs.addTab(b, "B");
    tabs.addTab(c, "C");
    tabs.setCurrentWidget(b);
    tabs.tabBar()->moveTab(0, 2);
    return tabs.widget(0) == b && tabs.widget(1) == c &&
           tabs.widget(2) == a && tabs.currentWidget() == b ? 0 : 1;
}
