package middleware

import (
	"encoding/json"
	"log"
	"math"
	"net/http"
	"regexp"
	"strconv"
	"time"
)

// ErrorResponse 错误响应
type ErrorResponse struct {
	Success bool   `json:"success"`
	Error   string `json:"error"`
}

// AuthMiddleware 认证中间件
func AuthMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// 跳过 /api/check 端点的认证
		if r.URL.Path == "/api/check" {
			next.ServeHTTP(w, r)
			return
		}

		// 验证 Content-Type（仅对POST/PUT/PATCH等需要body的请求）
		if r.Method == "POST" || r.Method == "PUT" || r.Method == "PATCH" {
			contentType := r.Header.Get("Content-Type")
			if contentType != "application/json" {
				sendError(w, "Content-Type必须为application/json", http.StatusBadRequest)
				return
			}
		}

		// 验证 X-Client-Key
		clientKey := r.Header.Get("X-Client-Key")
		if clientKey == "" {
			sendError(w, "缺少X-Client-Key请求头", http.StatusUnauthorized)
			return
		}

		// 验证 X-Client-Key 格式（SHA256哈希，64字符十六进制）
		matched, _ := regexp.MatchString("^[a-f0-9]{64}$", clientKey)
		if !matched {
			sendError(w, "X-Client-Key格式错误", http.StatusUnauthorized)
			return
		}

		// 验证 X-Request-Time
		requestTimeStr := r.Header.Get("X-Request-Time")
		if requestTimeStr == "" {
			sendError(w, "缺少X-Request-Time请求头", http.StatusUnauthorized)
			return
		}

		requestTime, err := strconv.ParseInt(requestTimeStr, 10, 64)
		if err != nil {
			sendError(w, "X-Request-Time格式错误", http.StatusBadRequest)
			return
		}

		// 验证时间戳（允许±5分钟误差）
		currentTime := time.Now().Unix()
		timeDiff := math.Abs(float64(currentTime - requestTime))
		if timeDiff > 300 { // 5分钟 = 300秒
			log.Printf("时间戳验证失败: 当前时间=%d, 请求时间=%d, 差值=%.0f秒", currentTime, requestTime, timeDiff)
			sendError(w, "请求时间戳无效或已过期", http.StatusUnauthorized)
			return
		}

		// 认证通过，继续处理
		next.ServeHTTP(w, r)
	})
}

// sendError 发送错误响应
func sendError(w http.ResponseWriter, message string, statusCode int) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(statusCode)
	json.NewEncoder(w).Encode(ErrorResponse{
		Success: false,
		Error:   message,
	})
}

