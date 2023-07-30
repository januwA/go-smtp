#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <curl/curl.h>
#include "smtp.h"

#define FREE_CHAR_PTR(name) \
  if (name != NULL)         \
  {                         \
    free(name);             \
    name = NULL;            \
  }

/**cpp通常由go管理内存，所以在这里不用释放内存*/
#define FREE_GO_CPP(name)          \
  if (name != NULL)                \
  {                                \
    for (char **i = name; *i; i++) \
    {                              \
      free(*i);                    \
      *i = NULL;                   \
    }                              \
  }

#define CHECK_RES()    \
  if (res != CURLE_OK) \
    goto exit_send;

struct MemPart *NewMemPart()
{
  struct MemPart *part = malloc(sizeof(struct MemPart));
  return part;
}

void DestroyMemPart(struct MemPart *part)
{
  if (part == NULL)
    return;

  // 释放header的每个字符串
  FREE_GO_CPP(part->headers);

  free(part);
}

struct SmtpMimeOptions *NewSmtpMimeOptions()
{
  struct SmtpMimeOptions *opt = malloc(sizeof(struct SmtpMimeOptions));
  return opt;
}

void DestroySmtpMimeOptions(struct SmtpMimeOptions *opt)
{
  if (opt == NULL)
    return;

  FREE_CHAR_PTR(opt->url);
  FREE_CHAR_PTR(opt->username);
  FREE_CHAR_PTR(opt->password);
  FREE_CHAR_PTR(opt->authzid);
  FREE_CHAR_PTR(opt->loginOptions);
  FREE_CHAR_PTR(opt->from_addr);
  FREE_CHAR_PTR(opt->text);
  FREE_CHAR_PTR(opt->html);
  FREE_CHAR_PTR(opt->html_type);
  FREE_CHAR_PTR(opt->mime_type);
  FREE_CHAR_PTR(opt->mime_encoder);
  FREE_CHAR_PTR(opt->ca_info);
  FREE_CHAR_PTR(opt->ca_path);
  FREE_CHAR_PTR(opt->custom_request);

  FREE_GO_CPP(opt->rcpt);
  FREE_GO_CPP(opt->http_header);
  FREE_GO_CPP(opt->filePaths);

  if (opt->mem_parts != NULL)
  {
    for (struct MemPart **i = opt->mem_parts; *i; i++)
      DestroyMemPart(*i);

    // 这个指针由go管理
    // free(opt->mem_parts);
    // opt->mem_parts = NULL;
  }

  free(opt);
}

CURLcode sendSmtp(struct SmtpMimeOptions *opt)
{
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *headers = NULL;
  struct curl_slist *recipients = NULL;
  curl_mime *mime = NULL;
  curl_mime *alt = NULL;
  int is_free_alt = 1;
  curl_mimepart *part = NULL;

  curl = curl_easy_init();
  if (curl)
  {
    if (opt->url != NULL)
    {
      res = curl_easy_setopt(curl, CURLOPT_URL, opt->url);
      CHECK_RES()
    }

    if (opt->username != NULL)
    {
      res = curl_easy_setopt(curl, CURLOPT_USERNAME, opt->username);
      CHECK_RES();
    }

    if (opt->password != NULL)
    {
      res = curl_easy_setopt(curl, CURLOPT_PASSWORD, opt->password);
      CHECK_RES();
    }

    if (opt->authzid != NULL)
    {
      res = curl_easy_setopt(curl, CURLOPT_SASL_AUTHZID, opt->authzid);
      CHECK_RES();
    }

    if (opt->loginOptions != NULL)
    {
      res = curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, opt->loginOptions);
      CHECK_RES();
    }

    /**使用 SSL / TLS 进行传输的请求*/
    if (opt->useSSL != 0)
    {
      res = curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
      CHECK_RES();

      res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, opt->SSL_VERIFY);
      CHECK_RES();

      res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, opt->SSL_VERIFY);
      CHECK_RES();

      res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, opt->SSL_VERIFY);
      CHECK_RES();
    }

    if (opt->ca_info != NULL)
    {
      // /path/to/certificate.pem
      res = curl_easy_setopt(curl, CURLOPT_CAINFO, opt->ca_info);
      CHECK_RES();
    }

    if (opt->ca_info_blob != NULL)
    {
      struct curl_blob blob;
      blob.data = opt->ca_info_blob;
      blob.len = opt->ca_info_blob_len;
      blob.flags = opt->ca_info_blob_flags;
      res = curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);
      CHECK_RES();
    }

    if (opt->ca_path != NULL)
    {
      res = curl_easy_setopt(curl, CURLOPT_CAPATH, opt->ca_path);
      res = curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, opt->ca_path);
      CHECK_RES();
    }

    /** from */
    if (opt->from_addr != NULL)
    {
      res = curl_easy_setopt(curl, CURLOPT_MAIL_FROM, opt->from_addr);
      CHECK_RES();
    }

    /* rcpt */
    if (opt->rcpt != NULL)
    {
      for (char **i = opt->rcpt; *i; i++)
        recipients = curl_slist_append(recipients, *i);

      res = curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
      CHECK_RES();
    }

    /* header */
    if (opt->http_header != NULL)
    {
      for (char **i = opt->http_header; *i; i++)
        headers = curl_slist_append(headers, *i);

      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      CHECK_RES();
    }

    mime = curl_mime_init(curl);
    alt = curl_mime_init(curl);

    if (opt->html != NULL)
    {
      part = curl_mime_addpart(alt);
      res = curl_mime_data(part, opt->html, CURL_ZERO_TERMINATED);
      CHECK_RES();

      res = curl_mime_type(part, opt->html_type);
      CHECK_RES();

      res = curl_mime_encoder(part, opt->mime_encoder);
      CHECK_RES();
    }

    if (opt->text != NULL)
    {
      part = curl_mime_addpart(alt);
      res = curl_mime_data(part, opt->text, CURL_ZERO_TERMINATED);
      CHECK_RES();

      res = curl_mime_encoder(part, opt->mime_encoder);
      CHECK_RES();
    }

    part = curl_mime_addpart(mime);
    res = curl_mime_subparts(part, alt); /**之后不能再显示释放alt*/
    CHECK_RES();
    is_free_alt = 0;

    res = curl_mime_type(part, opt->mime_type);
    CHECK_RES();

    if (opt->filePaths != NULL)
    {
      for (char **i = opt->filePaths; *i; i++)
      {
        char *filename = *i;
        if (opt->debug)
          fprintf(stdout, "filename: %s\n", filename);

        part = curl_mime_addpart(mime);

        res = curl_mime_filedata(part, filename);
        CHECK_RES();

        res = curl_mime_encoder(part, opt->mime_encoder);
        CHECK_RES();
      }
    }

    if (opt->mem_parts != NULL)
    {
      for (struct MemPart **i = opt->mem_parts; *i; i++)
      {
        struct MemPart *memPart = *i;

        part = curl_mime_addpart(mime);
        res = curl_mime_data(part, memPart->mem, memPart->size);
        CHECK_RES();

        if (memPart->headers != NULL)
        {
          struct curl_slist *slist = NULL;
          for (char **h = memPart->headers; *h; h++)
            slist = curl_slist_append(slist, *h);

          if (slist != NULL)
          {
            res = curl_mime_headers(part, slist, 1);
            CHECK_RES();
          }
        }
      }
    }

    res = curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    CHECK_RES();

    if (opt->custom_request != NULL)
    {
      res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, opt->custom_request);
      CHECK_RES();
    }

    res = curl_easy_setopt(curl, CURLOPT_VERBOSE, opt->debug);
    CHECK_RES();

    res = curl_easy_perform(curl);
    CHECK_RES();
  }

exit_send:
  if (headers != NULL)
    curl_slist_free_all(headers);

  if (recipients != NULL)
    curl_slist_free_all(recipients);

  if (mime != NULL)
    curl_mime_free(mime);

  if (alt != NULL && is_free_alt)
    curl_mime_free(alt);

  if (curl != NULL)
    curl_easy_cleanup(curl);

  return res;
}