/*
For general Sqliteman copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Sqliteman
for which a new license (GPL+exception) is in place.
*/

#include <QCheckBox>
#include <QSqlQuery>
#include <QSqlError>

#include "altertabledialog.h"


AlterTableDialog::AlterTableDialog(QWidget * parent, const QString & tableName, const QString & schema)
	: TableEditorDialog(parent),
	m_table(tableName),
	m_schema(schema)
{
	update = false;

	ui.nameEdit->setText(tableName);
	ui.nameEdit->setDisabled(true);
	ui.databaseCombo->addItem(schema);
	ui.databaseCombo->setDisabled(true);
	ui.tabWidget->removeTab(1);
	ui.createButton->setText(tr("Alte&r"));
	ui.removeButton->setEnabled(false);
	setWindowTitle(tr("Alter Table"));

	ui.columnTable->insertColumn(4); // show if it's indexed
	QTableWidgetItem * captIx = new QTableWidgetItem(tr("Indexed"));
	ui.columnTable->setHorizontalHeaderItem(4, captIx);

	connect(ui.columnTable, SIGNAL(cellClicked(int, int)), this, SLOT(cellClicked(int,int)));

	resetStructure();
}

void AlterTableDialog::resetStructure()
{
	// obtain all indexed colums for DROP COLUMN checks
	foreach(QString index, Database::getObjects("index", m_schema).values(m_table))
	{
		foreach(QString indexColumn, Database::indexFields(index, m_schema))
			m_columnIndexMap[indexColumn].append(index);
	}

	// Initialize fields
	FieldList fields = Database::tableFields(m_table, m_schema);
	ui.columnTable->clearContents();
	ui.columnTable->setRowCount(fields.size());
	for(int i = 0; i < fields.size(); i++)
	{
		QTableWidgetItem * nameItem = new QTableWidgetItem(fields[i].name);
		QTableWidgetItem * typeItem = new QTableWidgetItem(fields[i].type);
		QTableWidgetItem * defItem = new QTableWidgetItem(fields[i].defval);
		QTableWidgetItem * ixItem = new QTableWidgetItem();
		ixItem->setFlags(Qt::ItemIsSelectable);
		if (m_columnIndexMap.contains(fields[i].name))
		{
			ixItem->setIcon(QIcon(QPixmap(QString(ICON_DIR) + "/index.png")));
			ixItem->setText(tr("Yes"));
		}
		else
			ixItem->setText(tr("No"));

		ui.columnTable->setItem(i, 0, nameItem);
		ui.columnTable->setItem(i, 1, typeItem);
		QCheckBox *nn = new QCheckBox(this);
		nn->setCheckState(fields[i].notnull ? Qt::Checked : Qt::Unchecked);
		ui.columnTable->setCellWidget(i, 2, nn);
		ui.columnTable->setItem(i, 3, defItem);
		ui.columnTable->setItem(i, 4, ixItem);
	}

	protectedRows = ui.columnTable->rowCount();
	ui.columnTable->resizeColumnsToContents();
}

void AlterTableDialog::createButton_clicked()
{
	// handle add columns
	if (alterTable())
	{
		update = true;
		resetStructure();
	}
}

bool AlterTableDialog::alterTable()
{
	// handle new columns
	DatabaseTableField f;
	QString sql("ALTER TABLE \"%1\".\"%2\" ADD COLUMN \"%3\" %4 %5 %6;");
	QString nn;
	QString def;
	QString fullSql;

	for(int i = protectedRows; i < ui.columnTable->rowCount(); i++)
	{
		f = getColumn(i);
		if (f.cid == -1)
			continue;

		nn = f.notnull ? " NOT NULL" : "";
// 		def = f.defval.length() > 0 ? QString(" DEFAULT (%1)").arg(f.defval) : "";
		def = getDefaultClause(f.defval);

		fullSql = sql.arg(ui.databaseCombo->currentText())
					.arg(m_table)
					.arg(f.name)
					.arg(f.type)
					.arg(nn)
					.arg(def);

		QSqlQuery query(fullSql, QSqlDatabase::database(SESSION_NAME));
		if(query.lastError().isValid())
		{
			ui.resultEdit->setText(tr("Error while altering table %1: %2.\n%3")
										.arg(m_table)
										.arg(query.lastError().databaseText())
										.arg(fullSql));
			return false;
		}
	}
	ui.resultEdit->setText(tr("Table altered successfully"));
	return true;
}

void AlterTableDialog::addField()
{
	TableEditorDialog::addField();
	checkChanges();
}

void AlterTableDialog::removeField()
{
	if (ui.columnTable->currentRow() < protectedRows)
		return;
	TableEditorDialog::removeField();
	checkChanges();
}

void AlterTableDialog::fieldSelected()
{
	if (ui.columnTable->currentRow() < protectedRows)
	{
		ui.removeButton->setEnabled(false);
		return;
	}
	TableEditorDialog::fieldSelected();
}

void AlterTableDialog::cellClicked(int row, int)
{
	if (row < protectedRows)
	{
		ui.removeButton->setEnabled(false);
		return;
	}
	TableEditorDialog::fieldSelected();
}

void AlterTableDialog::checkChanges()
{
	QString newName(ui.nameEdit->text().simplified());
	bool enable = false;
	if ((!newName.isEmpty() && m_table != newName) || protectedRows < ui.columnTable->rowCount())
		enable = true;
// 	ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
	ui.createButton->setEnabled(enable);
}

void AlterTableDialog::nameEdit_textChanged(const QString& s)
{
	checkChanges();
// 	NO CALL HERE! TableEditorDialog::nameEdit_textChanged(s);
}
