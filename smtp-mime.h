#pragma once

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
  struct MemPart
  {
    void *mem;
    size_t size;
    char **headers;
  };

  struct MemPart *NewMemPart();
  void DestroyMemPart(struct MemPart *);

  struct SmtpMimeOptions
  {
    /**smtp://smtp.qq.com:[port]*/
    char *url;
    char *username;
    char *password;
    /**AUTH=LOGIN*/
    char *loginOptions;
    /**[name] <foo\@qq.com>*/
    char *from;
    /**[name] <foo\@gmail.com>[, ...]*/
    char *to;
    char *subject;
    char *text;
    char *html;
    /**text/html*/
    char *html_type;
    /**{"a.jpg","b.jpg",NULL}*/
    char **filePaths;
    /**1*/
    long useSSL;
    /**0*/
    long SSL_VERIFY;
    long debug;
    /**multipart/mixed*/
    char *mime_type;
    /**base64*/
    char *mime_encoder;

    struct MemPart **mem_parts;
  };

  struct SmtpMimeOptions *NewSmtpMimeOptions();

  void DestroySmtpMimeOptions(struct SmtpMimeOptions *opt);

  CURLcode smtp_mime(struct SmtpMimeOptions *opt);
#ifdef __cplusplus
}
#endif // __cplusplus