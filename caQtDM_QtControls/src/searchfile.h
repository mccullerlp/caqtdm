/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2010 - 2014
 *
 *  Author:
 *    Anton Mezger
 *  Contact details:
 *    anton.mezger@psi.ch
 */

#ifndef SEARCHFILE_H
#define SEARCHFILE_H

#include <QString>
#include <QFileDialog>
#include <qtcontrols_global.h>

class QTCON_EXPORT searchFile:public QObject
{

public:
    searchFile(QString FileName);
    QString findFile();
    QString displayPath();
    /**
     * the name with a ".ui" suffix. Only what follows the last '.' behind the last path separator
     * counts as suffix, so "../dir/name" has none and becomes "../dir/name.ui"; an existing suffix
     * other than ui or prc (adl, edl, ...) is replaced.
     */
    static QString uiFileName(const QString &fileName);

private:
    QString _FileName;
};

#endif // SEARCHFILE_H
