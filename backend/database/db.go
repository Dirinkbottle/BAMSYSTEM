package database

import (
	"database/sql"
	"fmt"
	"log"

	_ "github.com/go-sql-driver/mysql"
)

var DB *sql.DB

// InitDB 初始化数据库连接
func InitDB(dsn string) error {
	var err error
	DB, err = sql.Open("mysql", dsn)
	if err != nil {
		return fmt.Errorf("数据库连接失败: %v", err)
	}

	// 测试连接
	if err = DB.Ping(); err != nil {
		return fmt.Errorf("数据库ping失败: %v", err)
	}

	// 设置连接池参数
	DB.SetMaxOpenConns(25)
	DB.SetMaxIdleConns(5)

	log.Println("数据库连接成功")

	// 自动创建表
	if err := createTables(); err != nil {
		return fmt.Errorf("创建数据表失败: %v", err)
	}

	return nil
}

// createTables 自动创建数据表
func createTables() error {
	createAccountsTable := `
	CREATE TABLE IF NOT EXISTS accounts (
		uuid VARCHAR(36) PRIMARY KEY,
		balance BIGINT UNSIGNED NOT NULL DEFAULT 0,
		created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
		updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
	`

	_, err := DB.Exec(createAccountsTable)
	if err != nil {
		return fmt.Errorf("创建accounts表失败: %v", err)
	}

	log.Println("数据表检查/创建完成")
	return nil
}

// CloseDB 关闭数据库连接
func CloseDB() {
	if DB != nil {
		DB.Close()
		log.Println("数据库连接已关闭")
	}
}

