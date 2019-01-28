/*
 * redpin.c - [Starting code for] a web-based manager of people and
 *            places.
 *
 * Based on:
 *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *      GET method to serve static and dynamic content.
 *   Tiny Web server
 *   Dave O'Hallaron
 *   Carnegie Mellon University
 */
#include "csapp.h"
#include "dictionary.h"
#include "more_string.h"

static void *doit(void *connfd_p);
static dictionary_t *read_requesthdrs(rio_t *rp);
static void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
static void clienterror(int fd, char *cause, char *errnum,
                        char *shortmsg, char *longmsg);
static void print_stringdictionary(dictionary_t *d);
static void serve_request(int fd, char *uri, dictionary_t *query);
static char *get_server_results(char *host, char *port, char is_place, char *person_or_place);

static dictionary_t *people; // person as key
static dictionary_t *places; // place as key

sem_t ready_sem, data_sem;

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  /* Don't kill the server if there's an error, because
     we want to survive errors due to a client. But we
     do want to report errors. */
  exit_on_error(0);

  /* Also, don't stop on broken connections: */
  Signal(SIGPIPE, SIG_IGN);

  people = make_dictionary(COMPARE_CASE_SENS, (free_proc_t)dictionary_free);
  places = make_dictionary(COMPARE_CASE_SENS, (free_proc_t)dictionary_free);

  Sem_init(&data_sem, 0, 1);
  Sem_init(&ready_sem, 0, 0);

  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd >= 0)
    {
      Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                  port, MAXLINE, 0);
      //printf("Accepted connection from (%s, %s)\n", hostname, port);

      pthread_t th;
      Pthread_create(&th, NULL, doit, &connfd);
      P(&ready_sem);
      Pthread_detach(th);
    }
  }

  dictionary_free(people);
  dictionary_free(places);
}

/*
 * doit - handle one HTTP request/response transaction
 */
void *doit(void *connfd_p)
{
  int fd = *(int *)connfd_p;
  V(&ready_sem);
  char buf[MAXLINE], *method, *uri, *version;
  rio_t rio;
  dictionary_t *headers, *query;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
    return NULL;
  //printf("%s", buf);

  if (!parse_request_line(buf, &method, &uri, &version))
  {
    clienterror(fd, method, "400", "Bad Request",
                "Redpin did not recognize the request");
  }
  else
  {
    if (strcasecmp(version, "HTTP/1.0") && strcasecmp(version, "HTTP/1.1"))
    {
      clienterror(fd, version, "501", "Not Implemented",
                  "Redpin does not implement that version");
    }
    else if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
      clienterror(fd, method, "501", "Not Implemented",
                  "Redpin does not implement that method");
    }
    else
    {
      headers = read_requesthdrs(&rio);

      /* Parse all query arguments into a dictionary */
      query = make_dictionary(COMPARE_CASE_SENS, free);

      parse_uriquery(uri, query);
      if (!strcasecmp(method, "POST"))
        read_postquery(&rio, headers, query);

      /* For debugging, print the dictionary */
      //print_stringdictionary(query);

      /* You'll want to handle different queries here,
         but the intial implementation always returns
         nothing: */
      P(&data_sem);
      serve_request(fd, uri, query);
      V(&data_sem);

      /* Clean up */
      dictionary_free(query);
      dictionary_free(headers);
    }

    /* Clean up status line */
    free(method);
    free(uri);
    free(version);
  }
  Close(fd);

  return NULL;
}

/*
 * read_requesthdrs - read HTTP request headers
 */
dictionary_t *read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    //printf("%s", buf);
    parse_header_line(buf, d);
    Rio_readlineb(rp, buf, MAXLINE);
  }

  return d;
}

void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest)
{
  char *len_str, *type, *buffer;
  int len;

  len_str = dictionary_get(headers, "Content-Length");
  len = (len_str ? atoi(len_str) : 0);

  type = dictionary_get(headers, "Content-Type");

  buffer = malloc(len + 1);
  Rio_readnb(rp, buffer, len);
  buffer[len] = 0;

  if (!strcasecmp(type, "application/x-www-form-urlencoded"))
  {
    parse_query(buffer, dest);
  }

  free(buffer);
}

static char *ok_header(size_t len, const char *content_type)
{
  char *len_str, *header;

  header = append_strings("HTTP/1.0 200 OK\r\n",
                          "Server: Redpin Web Server\r\n",
                          "Connection: close\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n",
                          "Content-type: ", content_type, "\r\n\r\n",
                          NULL);
  free(len_str);

  return header;
}

/*
 * serve_request - example request handler
 */
static void serve_request(int fd, char *uri, dictionary_t *query)
{
  size_t len = 0;
  char *body, *header;
  char is_badrequest = 0;
  body = NULL;

  if (starts_with("/counts", uri))
  {
    int bufsz = snprintf(NULL, 0, "%ld\n%ld\n", dictionary_count(people), dictionary_count(places));
    body = malloc(bufsz + 1);
    snprintf(body, bufsz + 1, "%ld\n%ld\n", dictionary_count(people), dictionary_count(places));
  }
  else if (starts_with("/reset", uri))
  {
    dictionary_free(people);
    dictionary_free(places);
    people = make_dictionary(COMPARE_CASE_SENS, (free_proc_t)dictionary_free);
    places = make_dictionary(COMPARE_CASE_SENS, (free_proc_t)dictionary_free);
    body = strdup("0\n0\n");
  }
  else if (starts_with("/people", uri))
  {
    if (dictionary_count(query) == 0)
    {
      const char **keys = dictionary_keys(people);
      body = join_strings(keys, '\n');
      Free(keys);
    }
    else
    {
      char *place = dictionary_get(query, "place");
      if (place)
      {
        dictionary_t *people_of_place = dictionary_get(places, place);
        if (people_of_place)
        {
          const char **keys = dictionary_keys(people_of_place);
          body = join_strings(keys, '\n');
          Free(keys);
        }
      }
      else
      {
        is_badrequest = 1;
      }
    }
  }
  else if (starts_with("/places", uri))
  {
    if (dictionary_count(query) == 0)
    {
      const char **keys = dictionary_keys(places);
      body = join_strings(keys, '\n');
      Free(keys);
    }
    else
    {
      char *person = dictionary_get(query, "person");
      if (person)
      {
        dictionary_t *places_of_person = dictionary_get(people, person);
        if (places_of_person)
        {
          const char **keys = dictionary_keys(places_of_person);
          body = join_strings(keys, '\n');
          Free(keys);
        }
      }
      else
      {
        is_badrequest = 1;
      }
    }
  }
  else if (starts_with("/pin", uri))
  {
    char *query_people = dictionary_get(query, "people");
    char *query_places = dictionary_get(query, "places");

    if (query_people && query_places)
    {
      char **people_list = split_string(query_people, '\n');
      char **places_list = split_string(query_places, '\n');

      int i, j;
      for (i = 0;; i++)
      {
        char *person = people_list[i];
        if (person == NULL)
          break;

        dictionary_t *places_of_person = dictionary_get(people, person);
        if (!places_of_person)
        {
          places_of_person = make_dictionary(COMPARE_CASE_SENS, Free);
          //P(&people_sem);
          dictionary_set(people, person, places_of_person);
          //V(&people_sem);
        }
        for (j = 0;; j++)
        {
          char *place = places_list[j];
          if (place == NULL)
            break;

          //P(&people_sem);
          dictionary_set(places_of_person, place, strdup(place));
          //V(&people_sem);

          dictionary_t *people_of_place = dictionary_get(places, place);
          if (!people_of_place)
          {
            people_of_place = make_dictionary(COMPARE_CASE_SENS, Free);
            //P(&places_sem);
            dictionary_set(places, place, people_of_place);
            //V(&places_sem);
          }
          //P(&places_sem);
          dictionary_set(people_of_place, person, strdup(person));
          //V(&places_sem);
        }
      }

      for (i = 0;; i++)
      {
        char *person = people_list[i];
        if (person == NULL)
          break;
        Free(person);
      }
      for (i = 0;; i++)
      {
        char *place = places_list[i];
        if (place == NULL)
          break;
        Free(place);
      }

      Free(people_list);
      Free(places_list);

      int bufsz = snprintf(NULL, 0, "%ld\n%ld\n", dictionary_count(people), dictionary_count(places));
      body = malloc(bufsz + 1);
      snprintf(body, bufsz + 1, "%ld\n%ld\n", dictionary_count(people), dictionary_count(places));
    }
    else
    {
      is_badrequest = 1;
    }
  }
  else if (starts_with("/unpin", uri))
  {
    char *query_people = dictionary_get(query, "people");
    char *query_places = dictionary_get(query, "places");

    if (query_people && query_places)
    {
      char **people_list = split_string(query_people, '\n');
      char **places_list = split_string(query_places, '\n');

      int i, j;
      for (i = 0;; i++)
      {
        char *person = people_list[i];
        if (person == NULL)
          break;

        dictionary_t *places_of_person = dictionary_get(people, person);
        if (places_of_person)
        {
          for (j = 0;; j++)
          {
            char *place = places_list[j];
            if (place == NULL)
              break;

            if (dictionary_has_key(places_of_person, place))
            {
              //P(&people_sem);
              dictionary_remove(places_of_person, place);
              //V(&people_sem);
              if (dictionary_count(places_of_person) == 0)
              {
                //P(&people_sem);
                dictionary_remove(people, person);
                //V(&people_sem);
              }
            }

            dictionary_t *people_of_place = dictionary_get(places, place);
            if (people_of_place && dictionary_has_key(people_of_place, person))
            {
              //P(&places_sem);
              dictionary_remove(people_of_place, person);
              //V(&places_sem);
              if (dictionary_count(people_of_place) == 0)
              {
                //P(&places_sem);
                dictionary_remove(places, place);
                //V(&places_sem);
              }
            }
          }
        }
      }

      for (i = 0;; i++)
      {
        char *person = people_list[i];
        if (person == NULL)
          break;
        Free(person);
      }
      for (i = 0;; i++)
      {
        char *place = places_list[i];
        if (place == NULL)
          break;
        Free(place);
      }

      Free(people_list);
      Free(places_list);

      int bufsz = snprintf(NULL, 0, "%ld\n%ld\n", dictionary_count(people), dictionary_count(places));
      body = malloc(bufsz + 1);
      snprintf(body, bufsz + 1, "%ld\n%ld\n", dictionary_count(people), dictionary_count(places));
    }
    else
    {
      is_badrequest = 1;
    }
  }
  else if (starts_with("/copy", uri))
  {
    char *query_host = dictionary_get(query, "host");
    char *query_port = dictionary_get(query, "port");
    char *query_person = dictionary_get(query, "person");
    char *query_place = dictionary_get(query, "place");
    char *query_as = dictionary_get(query, "as");
    char is_place = query_person != NULL ? 1 : 0;

    if (is_place && query_host && query_port && query_person && query_as)
    {
      V(&data_sem);
      char *response = get_server_results(query_host, query_port, is_place, query_person);
      P(&data_sem);
      if (response == NULL)
      {
        is_badrequest = 1;
      }
      else
      {
        char **places_list = split_string(response, '\n');
        dictionary_t *places_of_as = dictionary_get(people, query_as);
        if (!places_of_as)
        {
          places_of_as = make_dictionary(COMPARE_CASE_SENS, Free);
          //P(&people_sem);
          dictionary_set(people, query_as, places_of_as);
          //V(&people_sem);
        }

        int i;
        for (i = 0;; i++)
        {
          char *place = places_list[i];
          if (place == NULL)
            break;

          //P(&people_sem);
          dictionary_set(places_of_as, place, strdup(place));
          //V(&people_sem);

          dictionary_t *people_of_place = dictionary_get(places, place);
          if (!people_of_place)
          {
            people_of_place = make_dictionary(COMPARE_CASE_SENS, Free);
            //P(&places_sem);
            dictionary_set(places, place, people_of_place);
            //V(&places_sem);
          }
          //P(&places_sem);
          dictionary_set(people_of_place, query_as, strdup(query_as));
          //V(&places_sem);
        }

        for (i = 0;; i++)
        {
          char *place = places_list[i];
          if (place == NULL)
            break;
          Free(place);
        }

        Free(places_list);
      }
      Free(response);
    }
    else if (!is_place && query_host && query_port && query_place && query_as)
    {
      V(&data_sem);
      char *response = get_server_results(query_host, query_port, is_place, query_place);
      P(&data_sem);
      if (response == NULL)
      {
        is_badrequest = 1;
      }
      else
      {
        char **people_list = split_string(response, '\n');
        dictionary_t *people_of_as = dictionary_get(places, query_as);
        if (!people_of_as)
        {
          people_of_as = make_dictionary(COMPARE_CASE_SENS, Free);
          //P(&places_sem);
          dictionary_set(places, query_as, people_of_as);
          //V(&places_sem);
        }

        int i;
        for (i = 0;; i++)
        {
          char *person = people_list[i];
          if (person == NULL)
            break;

          //P(&places_sem);
          dictionary_set(people_of_as, person, strdup(person));
          //V(&places_sem);

          dictionary_t *places_of_person = dictionary_get(people, person);
          if (!places_of_person)
          {
            places_of_person = make_dictionary(COMPARE_CASE_SENS, Free);

            //P(&people_sem);
            dictionary_set(people, person, places_of_person);
            //V(&people_sem);
          }
          //P(&people_sem);
          dictionary_set(places_of_person, query_as, strdup(query_as));
          //V(&people_sem);
        }

        for (i = 0;; i++)
        {
          char *person = people_list[i];
          if (person == NULL)
            break;
          Free(person);
        }

        Free(people_list);
      }
      Free(response);
    }
    else
    {
      is_badrequest = 1;
    }

    int bufsz = snprintf(NULL, 0, "%ld\n%ld\n", dictionary_count(people), dictionary_count(places));
    body = malloc(bufsz + 1);
    snprintf(body, bufsz + 1, "%ld\n%ld\n", dictionary_count(people), dictionary_count(places));
  }
  else
  {
    is_badrequest = 1;
  }

  if (is_badrequest)
  {
    clienterror(fd, "POST", "400", "Bad Request",
                "Redpin did not recognize the request");
  }
  else
  {
    if (body != NULL)
    {
      len = strlen(body);
    }

    /* Send response headers to client */
    header = ok_header(len, "text/plain; charset=utf-8");
    Rio_writen(fd, header, strlen(header));
    //printf("Response headers:\n");
    //printf("%s", header);

    free(header);

    /* Send response body to client */
    Rio_writen(fd, body, len);
  }
  if (body != NULL)
  {
    free(body);
  }
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  size_t len;
  char *header, *body, *len_str;

  body = append_strings("<html><title>Redpin Error</title>",
                        "<body bgcolor="
                        "ffffff"
                        ">\r\n",
                        errnum, " ", shortmsg,
                        "<p>", longmsg, ": ", cause,
                        "<hr><em>Redpin Server</em>\r\n",
                        NULL);
  len = strlen(body);

  /* Print the HTTP response */
  header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg, "\r\n",
                          "Content-type: text/html; charset=utf-8\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n\r\n",
                          NULL);
  free(len_str);

  Rio_writen(fd, header, strlen(header));
  Rio_writen(fd, body, len);

  free(header);
  free(body);
}

static void print_stringdictionary(dictionary_t *d)
{
  int i;
  const char **keys;

  keys = dictionary_keys(d);

  for (i = 0; keys[i] != NULL; i++)
  {
    printf("%s=%s\n",
           keys[i],
           (const char *)dictionary_get(d, keys[i]));
  }
  printf("\n");

  free(keys);
}

static char *get_server_results(char *host, char *port, char is_place, char *person_or_place)
{
  int fd = Open_clientfd(host, port);

  char *query = append_strings(is_place ? "person=" : "place=", person_or_place, NULL);
  // char *encoded = query_encode(query);
  char *url = append_strings(is_place ? "/places?" : "/people?", query, NULL);
  char *request = append_strings("GET ", url, " HTTP/1.1", "\r\n\r\n", NULL);
  Rio_writen(fd, request, strlen(request));

  Free(query);
  // Free(encoded);
  Free(url);
  Free(request);

  char buf[MAXLINE];
  rio_t rio;
  Rio_readinitb(&rio, fd);
  int content_length = MAXLINE;
  int i = 0;

  while (Rio_readlineb(&rio, buf, MAXLINE) > 2)
  {
    //printf("%s", buf);

    if (i == 0)
    {
      char **status = split_string(buf, ' ');
      int status_code = atoi(status[1]);

      int j;
      for (j = 0;; j++)
      {
        char *str = status[j];
        if (str == NULL)
          break;
        Free(str);
      }
      Free(status);

      if (status_code != 200)
      {
        return NULL;
      }
    }

    if (starts_with("Content-length", buf) || starts_with("Content-Length", buf))
    {
      char *len_str = buf + 16;
      content_length = atoi(len_str);
    }

    i++;
  }

  char *response_buf = calloc(content_length + 2, 1);
  Rio_readnb(&rio, response_buf, content_length + 1);
  response_buf[content_length + 1] = 0;

  Close(fd);
  return response_buf;
}
