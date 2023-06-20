package gosmtpcurl

import (
	"os"
	"testing"
)

func TestSmtpMime(t *testing.T) {
	opt := NewSmtpOptions()
	// opt.Url = "smtps://smtp.qq.com"
	// opt.UseSSL = 1

	// opt.Url = "smtp://smtp.qq.com:587"
	opt.Url = "smtp://smtp.qq.com"

	opt.Username = "1641845087@qq.com"
	opt.Password = os.Getenv("QQ_EMAIL_PWD")

	// opt.From = "<1641845087@qq.com>"
	opt.From = "16418 <1641845087@qq.com>"

	// opt.To = "<ajanuw1995@gmail.com>"
	// opt.To = "foo <ajanuw1995@gmail.com>"
	opt.To = "foo <ajanuw1995@gmail.com>, bar <ajanuw1641845087@sina.com>"

	opt.Subject = "hello"

	// opt.Text = "<h1 style=\"color: red;\">Hello World</h1>"

	opt.Html = "<h1 style=\"color: red;\">Hello World</h1>"

	opt.FilePaths = []string{
		"/mnt/c/Users/16418/Pictures/a.jpg",
		"/mnt/c/Users/16418/Pictures/b.jpg",
		"/mnt/c/Users/16418/Pictures/f.gif",
	}

	opt.MemParts = []MemPart{
		{
			Mem:  []byte("abc"),
			Size: 2,
			Headers: []string{
				"Content-Disposition: attachment; filename=\"a.txt\"",
				"Content-Type: inode/x-empty",
			},
		},
	}

	opt.Debug = 1

	err := SmtpMime(opt)
	if err != nil {
		t.Error(err)
	}
}
