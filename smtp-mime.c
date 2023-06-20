#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <curl/curl.h>
#include "smtp-mime.h"

const char LEFT_ARROW = '<';
const char RIGHT_ARROW = '>';

static const char *FROM_LABEL = "From: ";
static const size_t FROM_LABE_LEN = 6;

static const char *TO_LABEL = "To: ";
static const size_t TO_LABE_LEN = 4;

static const char *SUBJECT_LABEL = "Subject: ";
static const size_t SUBJECT_LABE_LEN = 9;

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
  if (part->headers != NULL)
  {
    for (char **i = part->headers; *i; i++)
      free(*i);

    // 这个指针由go管理
    // free(part->headers);
    // part->headers = NULL;
  }
  free(part);
}

struct SmtpMimeOptions *NewSmtpMimeOptions()
{
  struct SmtpMimeOptions *opt = malloc(sizeof(struct SmtpMimeOptions));
  return opt;
}

#define FREE_CHAR_PTR(name) \
  if (name != NULL)         \
  {                         \
    free(name);             \
    name = NULL;            \
  }

void DestroySmtpMimeOptions(struct SmtpMimeOptions *opt)
{
  if (opt == NULL)
    return;

  FREE_CHAR_PTR(opt->url);
  FREE_CHAR_PTR(opt->username);
  FREE_CHAR_PTR(opt->password);
  FREE_CHAR_PTR(opt->loginOptions);
  FREE_CHAR_PTR(opt->from);
  FREE_CHAR_PTR(opt->to);
  FREE_CHAR_PTR(opt->subject);
  FREE_CHAR_PTR(opt->text);
  FREE_CHAR_PTR(opt->html);
  FREE_CHAR_PTR(opt->html_type);
  FREE_CHAR_PTR(opt->mime_encoder);
  FREE_CHAR_PTR(opt->mime_type);

  if (opt->filePaths != NULL)
  {
    for (char **i = opt->filePaths; *i; i++)
      free(*i);

    // 这个指针由go管理
    // free(opt->filePaths);
    // opt->filePaths = NULL;
  }

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

#define CHECK_RES()    \
  if (res != CURLE_OK) \
    goto exit_send;

CURLcode smtp_mime(struct SmtpMimeOptions *opt)
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
    res = curl_easy_setopt(curl, CURLOPT_URL, opt->url);
    CHECK_RES()

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

    /** from */
    char *from_addr_begin = strchr(opt->from, LEFT_ARROW);
    char *from_addr_end = strchr(from_addr_begin, RIGHT_ARROW);

    size_t from_addr_len = from_addr_end - from_addr_begin + sizeof(char);
    char from_addr[from_addr_len + sizeof(char)];

    memcpy(from_addr, from_addr_begin, from_addr_len);
    if (opt->debug)
      fprintf(stdout, "From Addr: %s\n", from_addr);

    res = curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from_addr);
    CHECK_RES();

    /* to */
    size_t begin = 0, end = 0;
    for (size_t i = 0; i < strlen(opt->to); i++)
    {
      char c = opt->to[i];
      if (c == LEFT_ARROW)
      {
        begin = i;
        continue;
      }

      if (c == RIGHT_ARROW)
      {
        end = i;
        size_t len = end - begin + sizeof(char);
        char to_addr[len + sizeof(char)];
        memcpy(to_addr, opt->to + begin, len);

        if (opt->debug)
          fprintf(stdout, "To Addr: %s\n", to_addr);

        recipients = curl_slist_append(recipients, to_addr);
      }
    }

    res = curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    CHECK_RES();

    /* 构建并设置消息头列表 */
    char _from[FROM_LABE_LEN + strlen(opt->from) + sizeof(char)];
    sprintf(_from, "%s%s", FROM_LABEL, opt->from);
    if (opt->debug)
      fprintf(stdout, "%s\n", _from);

    char _to[TO_LABE_LEN + strlen(opt->to) + sizeof(char)];
    sprintf(_to, "%s%s", TO_LABEL, opt->to);
    if (opt->debug)
      fprintf(stdout, "%s\n", _to);

    char _subject[SUBJECT_LABE_LEN + strlen(opt->subject) + sizeof(char)];
    sprintf(_subject, "%s%s", SUBJECT_LABEL, opt->subject);
    if (opt->debug)
      fprintf(stdout, "%s\n", _subject);

    headers = curl_slist_append(headers, _from);
    headers = curl_slist_append(headers, _to);
    headers = curl_slist_append(headers, _subject);
    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    CHECK_RES();

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
    is_free_alt = 0;
    CHECK_RES();

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
        curl_mime_data(part, memPart->mem, memPart->size);

        if (memPart->headers != NULL)
        {
          struct curl_slist *slist = NULL;
          for (char **h = memPart->headers; *h; h++)
            slist = curl_slist_append(slist, *h);
          if (slist != NULL)
            curl_mime_headers(part, slist, 1);
        }
      }
    }

    res = curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    CHECK_RES();

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