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
    char *authzid;
    /** AUTH=LOGIN AUTH=PLAIN */
    char *loginOptions;

    char *from_addr;
    char **rcpt;
    char **http_header;
    char *text;
    char *html;
    /**text/html*/
    char *html_type;

    /**{"a.jpg","b.jpg",NULL}*/
    char **filePaths;
    /**multipart/mixed*/
    char *mime_type;
    /**base64*/
    char *mime_encoder;
    struct MemPart **mem_parts;

    /**1*/
    long useSSL;
    /**0*/
    long SSL_VERIFY;
    char *ca_info;

    /**此选项将覆盖ca_info*/
    void *ca_info_blob;
    size_t ca_info_blob_len;
    /**copy 1, nocopy 0*/
    unsigned int ca_info_blob_flags;

    char *ca_path;

    char *custom_request;

    long debug;
  };

  struct SmtpMimeOptions *NewSmtpMimeOptions();

  void DestroySmtpMimeOptions(struct SmtpMimeOptions *opt);

  CURLcode smtp_mime(struct SmtpMimeOptions *opt);
#ifdef __cplusplus
}
#endif // __cplusplus