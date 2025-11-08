package main

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"

	"bamsystem-backend/config"
	"bamsystem-backend/database"
	"bamsystem-backend/handlers"
	"bamsystem-backend/middleware"

	"github.com/gorilla/mux"
)

func main() {
	log.Println("=== BAMSYSTEM 服务器启动 ===")

	// 加载配置文件
	if err := config.LoadConfig("config.json"); err != nil {
		log.Fatalf("加载配置文件失败: %v", err)
	}
	log.Println("配置文件加载成功")

	// 初始化数据库
	dsn := config.GlobalConfig.Database.GetDSN()
	if err := database.InitDB(dsn); err != nil {
		log.Fatalf("数据库初始化失败: %v", err)
	}
	defer database.CloseDB()

	// 创建路由器
	router := mux.NewRouter()

	// 应用认证中间件
	router.Use(middleware.AuthMiddleware)

	// 注册API路由
	router.HandleFunc("/api/check", handlers.CheckServerHandler).Methods("GET")
	router.HandleFunc("/api/accounts", handlers.GetAllAccountsHandler).Methods("GET")
	router.HandleFunc("/api/account/create", handlers.CreateAccountHandler).Methods("POST")
	router.HandleFunc("/api/account/deposit", handlers.DepositHandler).Methods("POST")
	router.HandleFunc("/api/account/withdraw", handlers.WithdrawHandler).Methods("POST")
	router.HandleFunc("/api/account/transfer", handlers.TransferHandler).Methods("POST")
	router.HandleFunc("/api/account/sync", handlers.SyncAccountHandler).Methods("POST")
	router.HandleFunc("/api/account/{uuid}", handlers.DeleteAccountHandler).Methods("DELETE")
	router.HandleFunc("/api/public_key", handlers.GetPublicKeyHandler).Methods("GET")

	// 服务器配置
	serverAddr := fmt.Sprintf(":%d", config.GlobalConfig.Server.Port)
	certFile := config.GlobalConfig.Server.CertFile
	keyFile := config.GlobalConfig.Server.KeyFile

	// 检查证书文件是否存在
	useHTTPS := true
	if _, err := os.Stat(certFile); os.IsNotExist(err) {
		log.Printf("警告: 证书文件 %s 不存在，将使用HTTP模式", certFile)
		useHTTPS = false
	}
	if _, err := os.Stat(keyFile); os.IsNotExist(err) {
		log.Printf("警告: 密钥文件 %s 不存在，将使用HTTP模式", keyFile)
		useHTTPS = false
	}

	// 启动服务器
	go func() {
		if useHTTPS {
			log.Printf("服务器启动在 HTTPS://0.0.0.0%s", serverAddr)
			if err := http.ListenAndServeTLS(serverAddr, certFile, keyFile, router); err != nil {
				log.Fatalf("HTTPS服务器启动失败: %v", err)
			}
		} else {
			log.Printf("服务器启动在 HTTP://0.0.0.0%s", serverAddr)
			if err := http.ListenAndServe(serverAddr, router); err != nil {
				log.Fatalf("HTTP服务器启动失败: %v", err)
			}
		}
	}()

	log.Println("=== 服务器运行中，按 Ctrl+C 停止 ===")

	// 优雅关闭
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)
	<-sigChan

	log.Println("\n=== 服务器正在关闭... ===")
	log.Println("再见！")
}

