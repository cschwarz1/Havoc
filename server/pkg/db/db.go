package db

import (
	"database/sql"

	_ "github.com/mattn/go-sqlite3"
)

type Database struct {
	sqlite *sql.DB
	path   string
}

func NewDatabase(path string) (*Database, error) {
	var (
		database *Database
		sqlite   *sql.DB
		err      error
	)

	sqlite, err = sql.Open("sqlite3", path)
	if err != nil {
		return nil, err
	}

	database = &Database{
		path:   path,
		sqlite: sqlite,
	}

	// prepare the database to be used in case
	// the database has just been created and
	// does not contain any kind of tables
	if err = database.prepare(); err != nil {
		return nil, err
	}

	return database, nil
}

// prepare the database to be used in case
// it has not been initialized for use
func (db *Database) prepare() error {
	var err error

	// do not allow multiple servers trying
	// to access the same database
	// db.sqlite.SetMaxOpenConns(1)

	// creat a database table for Agents if it doesn't exist yet
	if _, err = db.sqlite.Exec(
		`CREATE TABLE IF NOT EXISTS "Agents" (
			"uuid"		TEXT,
			"type"		TEXT,
			"parent"	TEXT,
			"metadata"	BLOB NOT NULL,
			"status"	TEXT,
			"note"		TEXT,
			"disabled"	INTEGER
		)`,
	); err != nil {
		return err
	}

	// creat a database table for agent console output if it doesn't exist yet
	if _, err = db.sqlite.Exec(
		`CREATE TABLE IF NOT EXISTS "AgentConsole" (
			"uuid" TEXT,
			"data" BLOB NOT NULL
		)`,
	); err != nil {
		return err
	}

	// creat a database table for Listeners if it doesn't exist yet
	if _, err = db.sqlite.Exec(
		`CREATE TABLE IF NOT EXISTS "Listeners" (
			"name"		TEXT,
			"protocol"	TEXT,
			"status"	TEXT,
			"config"	BLOB NOT NULL
		)`,
	); err != nil {
		return err
	}

	// creat a database table for Plugins if it doesn't exist yet
	if _, err = db.sqlite.Exec(
		`CREATE TABLE IF NOT EXISTS "Plugins" (
			"name"    TEXT,
			"type"	  TEXT,
			"version" TEXT,
			"path"	  TEXT
		)`,
	); err != nil {
		return err
	}

	return err
}

func (db *Database) Close() error {
	return db.sqlite.Close()
}
