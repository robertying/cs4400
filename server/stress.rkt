#lang racket/base
(require racket/cmdline
         net/url
         net/uri-codec
         racket/port
         racket/string
         racket/list
         racket/place)

;; Number of entries will be the product of these:
(define NUM-PARALLEL 4)     ; base parallelism
(define NUM-THREADS 10)     ; more concurrency

(define NUM-PEOPLE 4)
(define NUM-PLACES 100)

(define NUM-REPEATS 1)

(define PLACE-LENGTH 10) ; size of each entry

;; Sending lots of headers sometimes will
;; make some connections sluggish, maybe worth
;; a try:
(define MAX-JUNK-HEADERS 0)

(define-values (host port)
  (command-line
   #:once-each
   [("--lo") "Run a short, low-stress variant"
             (set! NUM-PEOPLE 2)
             (set! NUM-THREADS 2)
             (set! NUM-PLACES 2)]
   #:args (host port)
   (values host port)))

(module+ main
  (define results
    (map place-wait
         (for/list ([p NUM-PARALLEL])
           (define pl (run-group))
           (place-channel-put pl p)
           pl)))
  (unless (andmap zero? results)
    (raise-user-error 'stress "one or more parallel tests failed"))
  (printf "Test passed\n"))

(define person-1 "%%one%%")
(define person-2 "\">two?")

(define (run-group)
  (place
   pch
   (define p (sync pch))

   (define root-url
     (string->url
      (format "http://~a:~a/" host port)))

   (define pin-url (combine-url/relative root-url "pin"))
   (define unpin-url (combine-url/relative root-url "unpin"))

   ;; maps a person to a known subset of its places:
   (define placess (make-hash))

   ;; all new places created in this place:
   (define all-places (make-hash))

   (define (check-expected-places person [expected-places #f])
     (define places (hash-ref placess person))
     (call/input-url (struct-copy url (combine-url/relative root-url
                                                            "places")
                                  [query (list
                                          (cons 'person person))])
                     get-pure-port
                     (lambda (i)
                       (define reported-places (make-hash))
                       (for ([l (in-lines i 'linefeed)])
                         (when (not (hash-ref places l 'unknown))
                           (raise-user-error 'stress "~a: unexpected place for ~s: ~s" p person l))
                         (when (hash-ref reported-places l #f)
                           (raise-user-error 'stress "duplicate place for ~s: ~s" person l))
                         (hash-set! reported-places l #t))
                       (for ([expected-place (in-hash-keys (or expected-places places))]
                             #:when (hash-ref places expected-place))
                         (unless (hash-ref reported-places expected-place #f)
                           (raise-user-error 'stress "missing place for ~s: ~s" person expected-place))))))

   (define (encode-post-query alist)
     (string->bytes/utf-8 (alist->form-urlencoded alist)))

   (define (pin #:person person #:places places)
     (call/input-url (struct-copy url pin-url
                                  [query (list
                                          (cons 'people person))])
                     (lambda (url)
                       (post-pure-port url
                                       (encode-post-query (list (cons 'places places)))
                                       (cons
                                        "Content-Type: application/x-www-form-urlencoded"
                                        (for/list ([i (random (add1 MAX-JUNK-HEADERS))])
                                          (format "x-junk-~a: garbage" i)))))
                     port->string))

   (define (unpin #:person person #:places places)
     (call/input-url (struct-copy url unpin-url
                                  [query (list
                                          (cons 'people person))])
                     (lambda (url)
                       (post-pure-port url
                                       (encode-post-query (list (cons 'places places)))
                                       (cons
                                        "Content-Type: application/x-www-form-urlencoded"
                                        (for/list ([i (random (add1 MAX-JUNK-HEADERS))])
                                          (format "x-junk-~a: garbage" i)))))
                     port->string))

   (define (id->person j)
     (case j
       [(0) person-1]
       [(1) person-2]
       [else (format "u~a" j)]))

   (define place-person (format "p~a" p))

   (log-error "pinning")
   (for-each
    sync
    (for/list ([k NUM-THREADS])
      (thread
       (lambda ()
         (for ([j NUM-PEOPLE])
           (define person (id->person j))
           (define places (hash-ref! placess person make-hash))
           (define local-places (make-hash))
           (for ([r NUM-REPEATS])
             (define new-places
               (for/list ([i NUM-PLACES])
                 (format "f[~a,~a,~a,~a,~a]~a" p k j r i (make-string PLACE-LENGTH #\*))))
             (define all-new-places (string-join new-places "\n"))
             (pin #:person person #:places all-new-places)
             (pin #:person place-person #:places all-new-places)
             (unpin #:person person #:places all-new-places)
             (pin #:person person #:places all-new-places)
             (for ([new-place (in-list new-places)])
               (hash-set! places new-place #t)
               (hash-set! local-places new-place #t)
               (hash-set! all-places new-place #t)))
           (check-expected-places person local-places))))))

   (log-error "checking places")
   (for ([j NUM-PEOPLE])
     (check-expected-places (id->person j)))

   (log-error "unpinning")
   (for ([j NUM-PEOPLE])
     (define person (id->person j))
     (define places (hash-ref placess person))
     (define all
       (string-join
        (for/list ([place (in-hash-keys all-places)])
          (hash-set! places place #f)
          place)
        "\n"))
     (unpin #:person person #:places all))

   (log-error "checking places")
   (for ([j NUM-PEOPLE])
     (check-expected-places (id->person j)))
   
   (log-error "copying")
   (for ([j NUM-PEOPLE])
     (define person (id->person j))
     (define places (hash-ref! placess person make-hash))
     (void (call/input-url (struct-copy url (combine-url/relative root-url
                                                                  "copy")
                                        [query (list
                                                (cons 'person place-person)
                                                (cons 'as person)
                                                (cons 'host host)
                                                (cons 'port port))])
                           get-pure-port
                           port->string))
     (for ([place (in-hash-keys all-places)])
       (hash-set! places place #t)))

   (log-error "checking places")
   (for ([j NUM-PEOPLE])
     (check-expected-places (id->person j)))))
