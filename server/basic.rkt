#lang racket/base
(require racket/cmdline
         racket/string
         racket/set
         racket/list
         racket/format
         net/url
         net/head
         racket/port
         racket/tcp)

(define contact-self? #t)
(define stop-on-first-failure? #t)

(define-values (host port)
  (command-line
   #:once-each
   [("--no-self") "Don't test /copy from self"
    (set! contact-self? #f)]
   [("--seed") s "Seed random number generator"
    (random-seed (or (string->number s)
                     (raise-user-error 'test "bad seed: ~a" s)))]
   [("--keep-going") "Don't stop on first error"
    (set! stop-on-first-failure? #f)]
   #:args (host port)
   (values host port)))

(define person1 "alice")
(define person2 "bob")
(define person3 "carol")
(define person4 "dan")
(define person5 "eve")
(define person6 "frank")
(define person7 "grace")

(define place1 "merrill")
(define place2 "warnock")
(define place3 "union")
(define place4 "stadium")
(define place5 "library")
(define place6 "commons")

(define root-url
  (string->url
   (format "http://~a:~a/" host port)))

(define fail? #f)

(define (set->string s #:separator [separator "\n"])
  (apply ~a #:separator separator (set->list s)))

(define (string->set str)
  (define l (string-split str #rx"\n"))
  (define s (list->set l))
  (if (= (length l) (set-count s))
      s
      #f))

(define (message what
                 #:person [person #f]
                 #:people [people #f]
                 #:place [place #f]
                 #:places [places #f]
                 #:status-checker [status-checker must-succeed]
                 . more)
  (printf "Sending ~a" what)
  (when person (printf " ~a" person))
  (when people (printf " ~a" (set->string #:separator " " people)))
  (when place (printf " @ ~a" place))
  (when places (printf " @ ~a" (set->string #:separator " " places)))
  (for ([m (in-list more)])
    (printf " ~a=~a" (car m) (cdr m)))
  (newline)
  (flush-output)
  (call/input-url (struct-copy url (combine-url/relative root-url what)
                               [query (shuffle
                                       (append
                                        more
                                        (if person
                                            (list (cons 'person person))
                                            null)
                                        (if people
                                            (list (cons 'people (set->string people)))
                                            null)
                                        (if place
                                            (list (cons 'place place))
                                            null)
                                        (if places
                                            (list (cons 'places (set->string places)))
                                            null)))])
                  (lambda (url)
                    (define-values (i h) (get-pure-port/headers url #:status? #t))
                    (define h-i (open-input-string h))
                    (read h-i)
                    (status-checker (read h-i))
                    i)
                  port->string))

(define (check-status str num-people num-places)
  (define expected (format "~a\n~a\n" num-people num-places))
  (unless (equal? str expected)
    (fail "Expected result:\n~aGot result:\n~a"
          expected str)))

(define (check-set str s)
  (define got-s (string->set str))
  (cond
    [(not got-s)
     (fail "Result has repeated elements:\n~a\n" str)]
    [(not (equal? got-s s))
     (fail "Expected result set:\n~a\nGot result set:\n~a\n"
           (set->string s)
           (set->string got-s))]))

(define failed? #f)

(define (fail fmt . args)
  (define sep (make-string 70 #\-))
  (eprintf "~a\nFAILURE\n~a~a\n" sep (apply format fmt args) sep)
  (set! failed? #t)
  (when stop-on-first-failure?
    (exit 1)))

(define (must-succeed st)
  (unless (equal? st 200)
    (fail "Bad result status: ~a\n" st)))

(define (must-fail st)
  (when (equal? st 200)
    (fail "Expected failure status, got success: ~a\n" st)))

(define (section str)
  (printf "[[ ~a ]]\n" str)
  (flush-output))

;; ----------------------------------------
(section "Reset")

(check-status (message "reset") 0 0)
(check-status (message "counts") 0 0)
(check-set (message "people") (set))
(check-set (message "places") (set))

;; ----------------------------------------
(section "Adding a Person at a Place")

(check-status (message "pin" #:people (set person1) #:places (set place1))
              1 1)
(check-set (message "people") (set person1))
(check-set (message "places") (set place1))
(check-set (message "people" #:place place1) (set person1))
(check-set (message "places" #:person person1) (set place1))

(check-status (message "pin" #:people (set person1) #:places (set place2))
              1 2)
(check-status (message "pin" #:people (set person2) #:places (set place1))
              2 2)

(check-set (message "people" #:place place1) (set person1 person2))
(check-set (message "places" #:person person1) (set place1 place2))
(check-set (message "people" #:place place2) (set person1))
(check-set (message "places" #:person person2) (set place1))

(check-set (message "people" #:place place3) (set))
(check-set (message "places" #:person person3) (set))

;; ----------------------------------------
(section "Adding People at Places")

(check-status (message "pin" #:people (set person3 person4) #:places (set place3 place4 place5))
              4 5)

(check-set (message "people") (set person1 person2 person3 person4))
(check-set (message "places") (set place1 place2 place3 place4 place5))

(check-set (message "people" #:place place3) (set person3 person4))
(check-set (message "people" #:place place4) (set person3 person4))
(check-set (message "people" #:place place5) (set person3 person4))

(check-set (message "places" #:person person1) (set place1 place2))
(check-set (message "places" #:person person2) (set place1))

(check-status (message "pin" #:people (set person1 person2) #:places (set place1 place3 place4 place5))
              4 5)

(check-set (message "people" #:place place3) (set person1 person2 person3 person4))
(check-set (message "people" #:place place4) (set person1 person2 person3 person4))
(check-set (message "people" #:place place5) (set person1 person2 person3 person4))

(check-set (message "places" #:person person1) (set place1 place2 place3 place4 place5))
(check-set (message "places" #:person person2) (set place1 place3 place4 place5))

;; ----------------------------------------
(section "Use Place Name as Person")

(check-status (message "pin" #:people (set place2) #:places (set place1))
              5 5)
(check-set (message "places" #:person place2) (set place1))
(check-set (message "people" #:place place1) (set person1 person2 place2))

(check-set (message "people" #:place place2) (set person1))
(check-set (message "places" #:person place1) (set))

(check-status (message "unpin" #:people (set place2) #:places (set place1))
              4 5)

(check-set (message "places" #:person place2) (set))
(check-set (message "people" #:place place1) (set person1 person2))

(check-set (message "people" #:place place2) (set person1))
(check-set (message "places" #:person place1) (set))

;; ----------------------------------------
(section "Removing a Pin")

(check-status (message "unpin" #:people (set person1) #:places (set place2))
              4 4)
(check-status (message "unpin" #:people (set person6) #:places (set place1))
              4 4)
(check-status (message "unpin" #:people (set person1) #:places (set place6))
              4 4)
(check-status (message "unpin" #:people (set person5 person6) #:places (set place5 place6))
              4 4)

(check-set (message "people") (set person1 person2 person3 person4))
(check-set (message "places") (set place1 place3 place4 place5))

(check-status (message "unpin" #:people (set person1) #:places (set place1 place3 place4))
              4 4)
(check-status (message "unpin" #:people (set person1) #:places (set place5))
              3 4)

(check-set (message "people") (set person2 person3 person4))
(check-set (message "places") (set place1 place3 place4 place5))

(check-set (message "people" #:place place1) (set person2))
(check-set (message "people" #:place place2) (set))
(check-set (message "people" #:place place3) (set person2 person3 person4))
(check-set (message "people" #:place place4) (set person2 person3 person4))
(check-set (message "people" #:place place5) (set person2 person3 person4))

;; ----------------------------------------
(section "Copying from Another Server")

;; Make a server that always responds with "foo" and "bar"

(define (handle in out)
  ;; Discard the request header (up to blank line):
  (regexp-match #rx"(\r\n|^)\r\n" in)
  ;; Send reply, always the same:
  (display "HTTP/1.0 200 Okay\r\n" out)
  (display "Server: k\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: 8\r\n\r\n" out)
  (display "foo\nbar\n" out))

(define l (tcp-listen 0 5 #t "localhost"))
(define server-port (let ()
                      (define-values (local local-port remote remote-port) (tcp-addresses l #t))
                      local-port))

(void
 (thread (lambda ()
           (let loop ()
             (define-values (in out) (tcp-accept l))
             (thread (lambda ()
                       (handle in out)
                       (close-input-port in)
                       (close-output-port out)))
             (loop)))))

(check-status (message "copy"
                       #:person person1
                       (cons 'as person6)
                       (cons 'host "localhost")
                       (cons 'port (~a server-port)))
              4 6)

(check-set (message "places" #:person person6) (set "foo" "bar"))
(check-set (message "places" #:person person1) (set))
(check-set (message "people") (set person2 person3 person4 person6))

(check-status (message "unpin" #:people (set person6) #:places (set "foo"))
              4 5)
(check-status (message "unpin" #:people (set person6) #:places (set "bar"))
              3 4)
(check-set (message "places" #:person person6) (set))

(check-status (message "copy"
                       #:place place1
                       (cons 'as place6)
                       (cons 'host "localhost")
                       (cons 'port (~a server-port)))
              5 5)

(check-set (message "people" #:place place6) (set "foo" "bar"))
(check-set (message "people" #:place place1) (set person2))
(check-set (message "places") (set place1 place3 place4 place5 place6))

(check-status (message "unpin" #:people (set "foo" "bar") #:places (set place6))
              3 4)
(check-set (message "people" #:place place6) (set))

(when contact-self?
  ;; ----------------------------------------
  (section "Copying from Self")

  (check-status (message "pin" #:people (set person1) #:places (set place1 place2 place3))
                4 5)

  (check-status (message "copy"
                         #:person person1
                         (cons 'as person6)
                         (cons 'host host)
                         (cons 'port (~a port)))
                5 5)

  (check-set (message "places" #:person person6) (set place1 place2 place3))
  (check-set (message "places" #:person person1) (set place1 place2 place3))

  (check-status (message "copy"
                         #:place place2
                         (cons 'as place6)
                         (cons 'host host)
                         (cons 'port (~a port)))
                5 6)

  (check-set (message "people" #:place place6) (set person1 person6))
  (check-set (message "people" #:place place2) (set person1 person6))

  (check-status (message "unpin" #:people (set person1) #:places (set place2))
                5 6)

  (check-set (message "people" #:place place6) (set person1 person6))
  (check-set (message "people" #:place place2) (set person6))
  (check-set (message "places" #:person person6) (set place1 place2 place3 place6))
  (check-set (message "places" #:person person1) (set place1 place3 place6)))

;; ----------------------------------------
(section "Error Handling")

(void (message "bad" #:status-checker must-fail))

(void (message "pin" #:status-checker must-fail))
(void (message "pin" #:people (set person1) #:status-checker must-fail))
(void (message "pin" #:places (set place1) #:status-checker must-fail))

(void (message "unpin" #:status-checker must-fail))
(void (message "unpin" #:people (set person1) #:status-checker must-fail))
(void (message "unpin" #:places (set place1) #:status-checker must-fail))

(void (message "copy" #:status-checker must-fail))
(void (message "copy" #:person person1 #:status-checker must-fail))
(void (message "copy" #:person place1 #:status-checker must-fail))
(void (message "copy" #:person person1 #:status-checker must-fail
               (cons 'as person2)))
(void (message "copy" #:person place1 #:status-checker must-fail
               (cons 'as place2)))
(void (message "copy" #:person person1 #:status-checker must-fail
               (cons 'host "localhost")
               (cons 'port "80")))
(void (message "copy" #:person place1 #:status-checker must-fail
               (cons 'host "localhost")
               (cons 'port "80")))
(void (message "copy" #:person person1 #:status-checker must-fail
               (cons 'as person2)
               (cons 'port "80")))
(void (message "copy" #:person place1 #:status-checker must-fail
               (cons 'as place2)
               (cons 'port "80")))
(void (message "copy" #:person person1 #:status-checker must-fail
               (cons 'as person2)
               (cons 'host "localhost")))
(void (message "copy" #:person place1 #:status-checker must-fail
               (cons 'as place2)
               (cons 'host "localhost")))

;; ----------------------------------------
(section "Reset")

(check-status (message "reset") 0 0)
(check-status (message "counts") 0 0)
(check-set (message "people") (set))
(check-set (message "places") (set))

;; ----------------------------------------
(section "Done")

(cond
  [failed?
   (eprintf "Test failed\n")
   (exit 1)]
  [else
   (printf "Test passed\n")])

