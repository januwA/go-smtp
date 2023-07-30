package smtpcurl

/*
#cgo pkg-config: libcurl

#include <curl/curl.h>
#include "smtp.h"
*/
import "C"
import (
	"errors"
	"unsafe"
)

type MemPart struct {
	Mem     []byte
	Headers []string
}

type SmtpOptions struct {
	/*服务器地址*/
	Url          string
	Username     string
	Password     string
	AuthZid      string
	LoginOptions string

	FromAddr string
	/*SMTP 邮件收件人列表*/
	Rcpt       []string
	HttpHeader []string

	/*文本格式的内容*/
	Text string

	/*html格式的内容*/
	Html     string
	HtmlType string

	/*上传附件*/
	FilePaths   []string
	MimeType    string
	MimeEncoder string
	MemParts    []MemPart

	/*0 or 1*/
	UseSSL int

	/*0 or 1*/
	SSL_VERIFY int

	CaInfo string

	CaInfoBlob      []byte
	CaInfoBlobFlags int

	CaPath        string
	CustomRequest string

	/*0 or 1*/
	Debug int
}

func NewSmtpOptions() *SmtpOptions {
	return &SmtpOptions{
		LoginOptions:    "AUTH=LOGIN",
		Debug:           0,
		UseSSL:          1,
		SSL_VERIFY:      0,
		MimeEncoder:     "base64",
		MimeType:        "multipart/mixed",
		HtmlType:        "text/html",
		CaInfoBlobFlags: 0,
	}
}

/* 返回NULL结尾的char**，cpp由go管理，但char*需要手动清理 */
func strArr2CPP(arr []string) **C.char {
	paths := make([]*C.char, len(arr)+1)
	for i, s := range arr {
		paths[i] = C.CString(s)
	}
	return (**C.char)(unsafe.Pointer(&paths[0]))
}

func Send(options *SmtpOptions) error {
	opt := C.NewSmtpMimeOptions()

	defer func() {
		C.DestroySmtpMimeOptions(opt)
	}()

	if options.Url != "" {
		opt.url = C.CString(options.Url)
	}
	if options.Username != "" {
		opt.username = C.CString(options.Username)
	}
	if options.Password != "" {
		opt.password = C.CString(options.Password)
	}
	if options.LoginOptions != "" {
		opt.loginOptions = C.CString(options.LoginOptions)
	}
	if options.FromAddr != "" {
		opt.from_addr = C.CString(options.FromAddr)
	}
	if len(options.Rcpt) != 0 {
		opt.rcpt = strArr2CPP(options.Rcpt)
	}
	if len(options.HttpHeader) != 0 {
		opt.http_header = strArr2CPP(options.HttpHeader)
	}
	if options.Text != "" {
		opt.text = C.CString(options.Text)
	}
	if options.Html != "" {
		opt.html = C.CString(options.Html)
	}
	if options.HtmlType != "" {
		opt.html_type = C.CString(options.HtmlType)
	}
	if len(options.FilePaths) != 0 {
		opt.filePaths = strArr2CPP(options.FilePaths)
	}
	if options.MimeEncoder != "" {
		opt.mime_encoder = C.CString(options.MimeEncoder)
	}
	if options.MimeType != "" {
		opt.mime_type = C.CString(options.MimeType)
	}
	if len(options.MemParts) != 0 {
		memParts := make([]*C.struct_MemPart, len(options.MemParts)+1)
		for i, item := range options.MemParts {
			memPart := C.NewMemPart()
			memPart.mem = unsafe.Pointer(&item.Mem[0])
			memPart.size = C.ulong(len(item.Mem))
			memPart.headers = strArr2CPP(item.Headers)
			memParts[i] = memPart
		}
		memParts = append(memParts, nil)
		opt.mem_parts = (**C.struct_MemPart)(unsafe.Pointer(&memParts[0]))
	}

	opt.useSSL = C.long(options.UseSSL)
	opt.SSL_VERIFY = C.long(options.SSL_VERIFY)
	if options.CaInfo != "" {
		opt.ca_info = C.CString(options.CaInfo)
	}
	if options.CaPath != "" {
		opt.ca_path = C.CString(options.CaPath)
	}
	if len(options.CaInfoBlob) != 0 {
		opt.ca_info_blob = unsafe.Pointer(&options.CaInfoBlob[0])
		opt.ca_info_blob_len = C.ulong(len(options.CaInfoBlob))
		opt.ca_info_blob_flags = C.uint(options.CaInfoBlobFlags)
	}

	if options.CustomRequest != "" {
		opt.custom_request = C.CString(options.CustomRequest)
	}

	opt.debug = C.long(options.Debug)

	res := C.sendSmtp(opt)
	if res != 0 {
		return errors.New(C.GoString(C.curl_easy_strerror(res)))
	}
	return nil
}
