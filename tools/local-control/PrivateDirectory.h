// SPDX-License-Identifier: MIT
#pragma once
#include <QString>
// Create or validate private discovery storage using native access controls.
bool alloyPrivateDirectory(const QString& path, bool create, QString* error);
