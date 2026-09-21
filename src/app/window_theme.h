#pragma once

class QObject;
class QWindow;

namespace WindowTheme {

// Switches the native title bar of a top-level window to the dark Windows theme.
void applyDarkTitleBar(QWindow* window);

// Applies the dark title bar to every window of the application as soon as its native surface exists.
void installDarkTitleBar(QObject* application);

}  // namespace WindowTheme
