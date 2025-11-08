package handlers

import (
	"encoding/json"
	"io/ioutil"
	"log"
	"net/http"
	"regexp"
	"strings"

	"bamsystem-backend/config"
	"bamsystem-backend/models"

	"github.com/gorilla/mux"
)

// StandardResponse 标准响应
type StandardResponse struct {
	Success bool   `json:"success"`
	Message string `json:"message,omitempty"`
	Error   string `json:"error,omitempty"`
}

// BalanceResponse 带余额的响应
type BalanceResponse struct {
	Success bool   `json:"success"`
	Balance uint64 `json:"balance,omitempty"`
	Message string `json:"message,omitempty"`
	Error   string `json:"error,omitempty"`
}

// CreateAccountRequest 创建账户请求
type CreateAccountRequest struct {
	UUID      string `json:"uuid"`
	Balance   uint64 `json:"balance"`
	Timestamp int64  `json:"timestamp"`
}

// DepositRequest 存款请求
type DepositRequest struct {
	UUID      string `json:"uuid"`
	Amount    uint64 `json:"amount"`
	Timestamp int64  `json:"timestamp"`
}

// WithdrawRequest 取款请求
type WithdrawRequest struct {
	UUID      string `json:"uuid"`
	Amount    uint64 `json:"amount"`
	Timestamp int64  `json:"timestamp"`
}

// TransferRequest 转账请求
type TransferRequest struct {
	UUIDFrom  string `json:"uuid_from"`
	UUIDTo    string `json:"uuid_to"`
	Amount    uint64 `json:"amount"`
	Timestamp int64  `json:"timestamp"`
}

// SyncAccountRequest 同步账户请求
type SyncAccountRequest struct {
	UUID      string `json:"uuid"`
	Balance   uint64 `json:"balance"`
	Timestamp int64  `json:"timestamp"`
}

// CheckServerHandler 检查服务器状态
func CheckServerHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{"status": "Support"})
}

// CreateAccountHandler 创建账户
func CreateAccountHandler(w http.ResponseWriter, r *http.Request) {
	var req CreateAccountRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		sendErrorResponse(w, "请求参数错误", http.StatusBadRequest)
		return
	}

	// 验证UUID格式
	if !isValidUUID(req.UUID) {
		sendErrorResponse(w, "UUID格式错误", http.StatusBadRequest)
		return
	}

	// 检查账户是否已存在
	if models.AccountExists(req.UUID) {
		sendErrorResponse(w, "账户已存在", http.StatusConflict)
		return
	}

	// 创建账户
	if err := models.CreateAccount(req.UUID, req.Balance); err != nil {
		log.Printf("创建账户失败: %v", err)
		sendErrorResponse(w, "创建账户失败", http.StatusInternalServerError)
		return
	}

	sendSuccessResponse(w, "账户创建成功")
}

// DepositHandler 存款
func DepositHandler(w http.ResponseWriter, r *http.Request) {
	var req DepositRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		sendErrorResponse(w, "请求参数错误", http.StatusBadRequest)
		return
	}

	// 验证UUID格式
	if !isValidUUID(req.UUID) {
		sendErrorResponse(w, "UUID格式错误", http.StatusBadRequest)
		return
	}

	// 验证金额
	if req.Amount == 0 {
		sendErrorResponse(w, "存款金额必须大于0", http.StatusBadRequest)
		return
	}

	// 执行存款
	newBalance, err := models.Deposit(req.UUID, req.Amount)
	if err != nil {
		log.Printf("存款失败: %v", err)
		if strings.Contains(err.Error(), "不存在") {
			sendErrorResponse(w, "账户不存在", http.StatusNotFound)
		} else {
			sendErrorResponse(w, "存款失败", http.StatusInternalServerError)
		}
		return
	}

	sendBalanceResponse(w, newBalance, "存款成功")
}

// WithdrawHandler 取款
func WithdrawHandler(w http.ResponseWriter, r *http.Request) {
	var req WithdrawRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		sendErrorResponse(w, "请求参数错误", http.StatusBadRequest)
		return
	}

	// 验证UUID格式
	if !isValidUUID(req.UUID) {
		sendErrorResponse(w, "UUID格式错误", http.StatusBadRequest)
		return
	}

	// 验证金额
	if req.Amount == 0 {
		sendErrorResponse(w, "取款金额必须大于0", http.StatusBadRequest)
		return
	}

	// 执行取款
	newBalance, err := models.Withdraw(req.UUID, req.Amount)
	if err != nil {
		log.Printf("取款失败: %v", err)
		if strings.Contains(err.Error(), "余额不足") {
			sendErrorResponse(w, "余额不足", http.StatusBadRequest)
		} else if strings.Contains(err.Error(), "不存在") {
			sendErrorResponse(w, "账户不存在", http.StatusNotFound)
		} else {
			sendErrorResponse(w, "取款失败", http.StatusInternalServerError)
		}
		return
	}

	sendBalanceResponse(w, newBalance, "取款成功")
}

// TransferHandler 转账
func TransferHandler(w http.ResponseWriter, r *http.Request) {
	var req TransferRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		sendErrorResponse(w, "请求参数错误", http.StatusBadRequest)
		return
	}

	// 验证UUID格式
	if !isValidUUID(req.UUIDFrom) || !isValidUUID(req.UUIDTo) {
		sendErrorResponse(w, "UUID格式错误", http.StatusBadRequest)
		return
	}

	// 验证金额
	if req.Amount == 0 {
		sendErrorResponse(w, "转账金额必须大于0", http.StatusBadRequest)
		return
	}

	// 验证不能转账给自己
	if req.UUIDFrom == req.UUIDTo {
		sendErrorResponse(w, "不能向自己转账", http.StatusBadRequest)
		return
	}

	// 执行转账
	if err := models.Transfer(req.UUIDFrom, req.UUIDTo, req.Amount); err != nil {
		log.Printf("转账失败: %v", err)
		if strings.Contains(err.Error(), "余额不足") {
			sendErrorResponse(w, "转出账户余额不足", http.StatusBadRequest)
		} else if strings.Contains(err.Error(), "不存在") {
			sendErrorResponse(w, err.Error(), http.StatusNotFound)
		} else {
			sendErrorResponse(w, "转账失败", http.StatusInternalServerError)
		}
		return
	}

	sendSuccessResponse(w, "转账成功")
}

// DeleteAccountHandler 删除账户
func DeleteAccountHandler(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	uuid := vars["uuid"]

	// 验证UUID格式
	if !isValidUUID(uuid) {
		sendErrorResponse(w, "UUID格式错误", http.StatusBadRequest)
		return
	}

	// 删除账户
	if err := models.DeleteAccount(uuid); err != nil {
		log.Printf("删除账户失败: %v", err)
		if strings.Contains(err.Error(), "不存在") {
			sendErrorResponse(w, "账户不存在", http.StatusNotFound)
		} else {
			sendErrorResponse(w, "删除账户失败", http.StatusInternalServerError)
		}
		return
	}

	sendSuccessResponse(w, "账户已删除")
}

// SyncAccountHandler 同步账户
func SyncAccountHandler(w http.ResponseWriter, r *http.Request) {
	var req SyncAccountRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		sendErrorResponse(w, "请求参数错误", http.StatusBadRequest)
		return
	}

	// 验证UUID格式
	if !isValidUUID(req.UUID) {
		sendErrorResponse(w, "UUID格式错误", http.StatusBadRequest)
		return
	}

	// 同步账户
	if err := models.SyncAccount(req.UUID, req.Balance); err != nil {
		log.Printf("同步账户失败: %v", err)
		sendErrorResponse(w, "同步账户失败", http.StatusInternalServerError)
		return
	}

	sendSuccessResponse(w, "账户数据已同步")
}

// GetPublicKeyHandler 获取服务器公钥/证书
func GetPublicKeyHandler(w http.ResponseWriter, r *http.Request) {
	certPath := config.GlobalConfig.Server.CertFile
	
	// 读取证书文件
	certData, err := ioutil.ReadFile(certPath)
	if err != nil {
		log.Printf("读取证书文件失败: %v", err)
		sendErrorResponse(w, "无法读取证书文件", http.StatusInternalServerError)
		return
	}

	// 返回证书内容
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"certificate": string(certData),
	})
}

// GetAllAccountsHandler 获取所有账户列表
func GetAllAccountsHandler(w http.ResponseWriter, r *http.Request) {
	accounts, err := models.GetAllAccounts()
	if err != nil {
		log.Printf("获取账户列表失败: %v", err)
		sendErrorResponse(w, "获取账户列表失败", http.StatusInternalServerError)
		return
	}

	// 返回账户列表
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]interface{}{
		"success":  true,
		"count":    len(accounts),
		"accounts": accounts,
	})
}

// 辅助函数

// isValidUUID 验证UUID格式
func isValidUUID(uuid string) bool {
	// UUID v4 格式: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
	pattern := "^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$"
	matched, _ := regexp.MatchString(pattern, strings.ToLower(uuid))
	return matched
}

// sendSuccessResponse 发送成功响应
func sendSuccessResponse(w http.ResponseWriter, message string) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(StandardResponse{
		Success: true,
		Message: message,
	})
}

// sendErrorResponse 发送错误响应
func sendErrorResponse(w http.ResponseWriter, message string, statusCode int) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(statusCode)
	json.NewEncoder(w).Encode(StandardResponse{
		Success: false,
		Error:   message,
	})
}

// sendBalanceResponse 发送带余额的响应
func sendBalanceResponse(w http.ResponseWriter, balance uint64, message string) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(BalanceResponse{
		Success: true,
		Balance: balance,
		Message: message,
	})
}

