#include "Application.h"
#include <QCoreApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include "logmanager.h"

Application::Application(QObject *parent) :
    QObject(parent)
{
}

Application::~Application()
{
}




