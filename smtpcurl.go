package smtpcurl

/*
#cgo pkg-config: libcurl

#include <curl/curl.h>
#include "smtp-mime.h"
*/
import "C"
import (
	"errors"
	"unsafe"
)

type MemPart struct {
	Mem     []byte
	Size    uint64
	Headers []string
}

type SmtpOptions struct {
	/*服务器地址*/
	Url string

	From    string
	To      string
	Subject string

	Username     string
	Password     string
	LoginOptions string

	/*文本格式的内容*/
	Text string

	/*html格式的内容*/
	Html string

	/*上传附件*/
	FilePaths []string

	/*0 or 1*/
	Debug int

	/*0 or 1*/
	UseSSL int

	/*0 or 1*/
	SSL_VERIFY int

	MimeType    string
	MimeEncoder string
	HtmlType    string

	MemParts []MemPart
}

func NewSmtpOptions() *SmtpOptions {
	return &SmtpOptions{
		LoginOptions: "AUTH=LOGIN",
		Debug:        0,
		UseSSL:       1,
		SSL_VERIFY:   0,
		MimeEncoder:  "base64",
		MimeType:     "multipart/mixed",
		HtmlType:     "text/html",
	}
}

func stringArr2CharPtrPtr(arr []string) **C.char {
	paths := make([]*C.char, len(arr)+1)
	for i, s := range arr {
		paths[i] = C.CString(s)
	}
	paths = append(paths, nil)
	return (**C.char)(unsafe.Pointer(&paths[0]))
}

func SmtpMime(options *SmtpOptions) error {
	opt := C.NewSmtpMimeOptions()

	defer func() {
		C.DestroySmtpMimeOptions(opt)
	}()

	opt.url = C.CString(options.Url)
	opt.from = C.CString(options.From)
	opt.to = C.CString(options.To)
	opt.subject = C.CString(options.Subject)

	if options.Username != "" {
		opt.username = C.CString(options.Username)
	}
	if options.Password != "" {
		opt.password = C.CString(options.Password)
	}
	if options.LoginOptions != "" {
		opt.loginOptions = C.CString(options.LoginOptions)
	}

	if options.Text != "" {
		opt.text = C.CString(options.Text)
	}

	if options.Html != "" {
		opt.html = C.CString(options.Html)
	}

	if len(options.FilePaths) != 0 {
		opt.filePaths = stringArr2CharPtrPtr(options.FilePaths)
	}

	opt.debug = C.long(options.Debug)
	opt.useSSL = C.long(options.UseSSL)
	opt.SSL_VERIFY = C.long(options.SSL_VERIFY)

	if options.MimeEncoder != "" {
		opt.mime_encoder = C.CString(options.MimeEncoder)
	}
	if options.MimeType != "" {
		opt.mime_type = C.CString(options.MimeType)
	}
	if options.HtmlType != "" {
		opt.html_type = C.CString(options.HtmlType)
	}

	if len(options.MemParts) != 0 {
		memParts := make([]*C.struct_MemPart, len(options.MemParts)+1)
		for i, item := range options.MemParts {
			memPart := C.NewMemPart()
			memPart.mem = unsafe.Pointer(&item.Mem[0])
			memPart.size = C.ulong(item.Size)
			memPart.headers = stringArr2CharPtrPtr(item.Headers)
			memParts[i] = memPart
		}
		memParts = append(memParts, nil)
		opt.mem_parts = (**C.struct_MemPart)(unsafe.Pointer(&memParts[0]))
	}

	res := C.smtp_mime(opt)
	if res != 0 {
		return errors.New(C.GoString(C.curl_easy_strerror(res)))
	}
	return nil
}
