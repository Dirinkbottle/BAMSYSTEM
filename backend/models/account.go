package models

import (
	"database/sql"
	"fmt"
	"time"

	"bamsystem-backend/database"
)

// Account 账户模型
type Account struct {
	UUID      string    `json:"uuid"`
	Balance   uint64    `json:"balance"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`
}

// CreateAccount 创建账户
func CreateAccount(uuid string, balance uint64) error {
	query := "INSERT INTO accounts (uuid, balance) VALUES (?, ?)"
	_, err := database.DB.Exec(query, uuid, balance)
	if err != nil {
		return fmt.Errorf("创建账户失败: %v", err)
	}
	return nil
}

// GetAccount 获取账户信息
func GetAccount(uuid string) (*Account, error) {
	query := "SELECT uuid, balance, created_at, updated_at FROM accounts WHERE uuid = ?"
	row := database.DB.QueryRow(query, uuid)

	account := &Account{}
	err := row.Scan(&account.UUID, &account.Balance, &account.CreatedAt, &account.UpdatedAt)
	if err != nil {
		if err == sql.ErrNoRows {
			return nil, fmt.Errorf("账户不存在")
		}
		return nil, fmt.Errorf("查询账户失败: %v", err)
	}

	return account, nil
}

// UpdateBalance 更新账户余额
func UpdateBalance(uuid string, newBalance uint64) error {
	query := "UPDATE accounts SET balance = ? WHERE uuid = ?"
	result, err := database.DB.Exec(query, newBalance, uuid)
	if err != nil {
		return fmt.Errorf("更新余额失败: %v", err)
	}

	rows, err := result.RowsAffected()
	if err != nil {
		return fmt.Errorf("获取影响行数失败: %v", err)
	}

	if rows == 0 {
		return fmt.Errorf("账户不存在")
	}

	return nil
}

// Deposit 存款
func Deposit(uuid string, amount uint64) (uint64, error) {
	account, err := GetAccount(uuid)
	if err != nil {
		return 0, err
	}

	newBalance := account.Balance + amount
	if err := UpdateBalance(uuid, newBalance); err != nil {
		return 0, err
	}

	return newBalance, nil
}

// Withdraw 取款
func Withdraw(uuid string, amount uint64) (uint64, error) {
	account, err := GetAccount(uuid)
	if err != nil {
		return 0, err
	}

	if account.Balance < amount {
		return 0, fmt.Errorf("余额不足")
	}

	newBalance := account.Balance - amount
	if err := UpdateBalance(uuid, newBalance); err != nil {
		return 0, err
	}

	return newBalance, nil
}

// Transfer 转账（使用事务）
func Transfer(uuidFrom, uuidTo string, amount uint64) error {
	// 开始事务
	tx, err := database.DB.Begin()
	if err != nil {
		return fmt.Errorf("开始事务失败: %v", err)
	}

	// 确保事务正确结束
	defer func() {
		if err != nil {
			tx.Rollback()
		}
	}()

	// 检查转出账户余额
	var fromBalance uint64
	err = tx.QueryRow("SELECT balance FROM accounts WHERE uuid = ?", uuidFrom).Scan(&fromBalance)
	if err != nil {
		if err == sql.ErrNoRows {
			return fmt.Errorf("转出账户不存在")
		}
		return fmt.Errorf("查询转出账户失败: %v", err)
	}

	if fromBalance < amount {
		return fmt.Errorf("转出账户余额不足")
	}

	// 检查转入账户是否存在
	var toBalance uint64
	err = tx.QueryRow("SELECT balance FROM accounts WHERE uuid = ?", uuidTo).Scan(&toBalance)
	if err != nil {
		if err == sql.ErrNoRows {
			return fmt.Errorf("转入账户不存在")
		}
		return fmt.Errorf("查询转入账户失败: %v", err)
	}

	// 扣除转出账户余额
	_, err = tx.Exec("UPDATE accounts SET balance = balance - ? WHERE uuid = ?", amount, uuidFrom)
	if err != nil {
		return fmt.Errorf("扣除转出账户余额失败: %v", err)
	}

	// 增加转入账户余额
	_, err = tx.Exec("UPDATE accounts SET balance = balance + ? WHERE uuid = ?", amount, uuidTo)
	if err != nil {
		return fmt.Errorf("增加转入账户余额失败: %v", err)
	}

	// 提交事务
	if err = tx.Commit(); err != nil {
		return fmt.Errorf("提交事务失败: %v", err)
	}

	return nil
}

// DeleteAccount 删除账户
func DeleteAccount(uuid string) error {
	query := "DELETE FROM accounts WHERE uuid = ?"
	result, err := database.DB.Exec(query, uuid)
	if err != nil {
		return fmt.Errorf("删除账户失败: %v", err)
	}

	rows, err := result.RowsAffected()
	if err != nil {
		return fmt.Errorf("获取影响行数失败: %v", err)
	}

	if rows == 0 {
		return fmt.Errorf("账户不存在")
	}

	return nil
}

// SyncAccount 同步账户数据（创建或更新）
func SyncAccount(uuid string, balance uint64) error {
	query := `
		INSERT INTO accounts (uuid, balance) 
		VALUES (?, ?) 
		ON DUPLICATE KEY UPDATE balance = ?
	`
	_, err := database.DB.Exec(query, uuid, balance, balance)
	if err != nil {
		return fmt.Errorf("同步账户失败: %v", err)
	}
	return nil
}

// AccountExists 检查账户是否存在
func AccountExists(uuid string) bool {
	var exists bool
	query := "SELECT EXISTS(SELECT 1 FROM accounts WHERE uuid = ?)"
	err := database.DB.QueryRow(query, uuid).Scan(&exists)
	return err == nil && exists
}

// GetAllAccounts 获取所有账户
func GetAllAccounts() ([]Account, error) {
	query := "SELECT uuid, balance, created_at, updated_at FROM accounts ORDER BY created_at DESC"
	rows, err := database.DB.Query(query)
	if err != nil {
		return nil, fmt.Errorf("查询账户列表失败: %v", err)
	}
	defer rows.Close()

	var accounts []Account
	for rows.Next() {
		var acc Account
		err := rows.Scan(&acc.UUID, &acc.Balance, &acc.CreatedAt, &acc.UpdatedAt)
		if err != nil {
			return nil, fmt.Errorf("扫描账户数据失败: %v", err)
		}
		accounts = append(accounts, acc)
	}

	if err = rows.Err(); err != nil {
		return nil, fmt.Errorf("遍历账户数据失败: %v", err)
	}

	return accounts, nil
}

