#lang racket/base
(require racket/cmdline
         racket/list
         racket/system
         racket/format)

(define verbose? #f)
(define fail-ok? #t)
(define ctl-c-ok? #t)
(define stop-on-error? #f)

(command-line
 #:once-each
 [("-v" "--verbose")
  "Print information about running test"
  (set! verbose? #t)]
 [("--no-fail")
  "Try only non-failing player programs"
  (set! fail-ok? #f)]
 [("--no-ctl-c")
  "Don't run tests of Ctl-C behavior"
  (set! ctl-c-ok? #f)]
 [("--stop-on-error")
  "Stop running tests on the first test failure"
  (set! stop-on-error? #t)]
 #:args ()
 (void))

(define failed? #f)

(define (run #:interrupt? [interrupt? #f]
             #:for-match? [for-match? #f]
             seed rounds . players+results)
  (define n (/ (length players+results) 4))
  (define players (take players+results n))
  (define o (open-output-bytes))
  (define e (open-output-bytes))
  (define args (list* (~a seed) (~a rounds) players))
  (when verbose?
    (printf "Trying run_tournament ~a~a\n" (apply ~a #:separator " " args)
            (cond
              [interrupt? " [with Ctl-C]"]
              [for-match? " [to match Ctl-C]"]
              [else ""])))
  (parameterize ([subprocess-group-enabled #t])
    (define p (apply process*/ports o (current-input-port) e "run_tournament" args))
    (define proc (list-ref p 4))
    (when interrupt?
      (sleep 0.5)
      (proc 'interrupt))
    (proc 'wait))
  (define e-bstr (get-output-bytes e))
  (unless (equal? e-bstr #"")
    (printf "[The following output to stderr is treated as a test failure]\n")
    (flush-output (current-output-port))
    (write-bytes e-bstr (current-error-port))
    (flush-output (current-error-port))
    (set! failed? #t))
  (define last-lines-bstr (get-last-lines (add1 n) (get-output-bytes o)))
  (unless (regexp-match? (build-result-regexp (if interrupt? '? rounds)
                                              (drop players+results n))
                         last-lines-bstr)
    (eprintf "End of output does not match expected.\n")
    (eprintf "Test case: ./run_tournament ~a\n" (apply ~a #:separator " " args))
    (eprintf "Got output:\n")
    (flush-output (current-output-port))
    (write-bytes last-lines-bstr (current-error-port))
    (flush-output (current-error-port))
    (eprintf "Expected output:\n")
    (eprintf (build-result-output (if interrupt? '? rounds) (drop players+results n)))
    (set! failed? #t))
  (when (and failed? stop-on-error?)
    (exit 1))
  (when (and interrupt? (not failed?))
    ;; Try again to see if the results match interrupting
    (define results (for/list ([v (in-port read (open-input-bytes last-lines-bstr))]) v))
    (apply run seed (car results) (append players (cdr results)) #:for-match? #t)))
    

(define (get-last-lines n bstr)
  (define lines
    (reverse
     (for/list ([l (in-bytes-lines (open-input-bytes bstr))])
       l)))
  (define keep-lines
    (reverse (take lines (min n (length lines)))))
  (apply bytes-append (append (add-between keep-lines #"\n") '(#"\n"))))

(define (build-result-regexp n results)
  (define (rx v) (if (eq? v '?) "[0-9]*" v))
  (string-append
   "\\s*0*" (~a (rx n)) "\n"
   (let loop ([results results])
     (cond
       [(null? results) ""]
       [else (string-append
              (~a "\\s*0*" (rx (car results)) " 0*" (rx (cadr results)) " 0*" (rx (caddr results)) "\n")
              (loop (cdddr results)))]))))

(define (build-result-output n results)
  (string-append
   (~a n) "\n"
   (let loop ([results results])
     (cond
       [(null? results) ""]
       [else (string-append
              (~a (car results) " " (cadr results) " " (caddr results) "\n")
              (loop (cdddr results)))]))))

(define scan_player "scan_player")
(define step_player "step_player")
(define jump_player "jump_player")
(define fumble_player "fumble_player")
(define garbled_player "garbled_player")
(define distracted_player "distracted_player")
(define unreliable_player "unreliable_player")

(run 42 1 step_player
     1 0 0)
(run 42 100 step_player
     100 0 0)
(run 42 100 step_player jump_player
     9 91 0
     91 9 0)
(run 43 62 step_player jump_player scan_player
     5 57 0
     57 5 0
     0 62 0)
(run 77 100 jump_player jump_player jump_player jump_player jump_player
     18 82 0
     26 74 0
     21 79 0
     19 81 0
     16 84 0)

(when fail-ok?
  (run 79 10 fumble_player
       10 0 10)
  (run 79 10 garbled_player
       0 10 10)
  (run 79 10 distracted_player
       0 10 10)
  (run 87 100 unreliable_player
       17 83 85)
  (run 79 50 fumble_player jump_player
       3 47 3
       47 3 0)
  (run 83 50 fumble_player jump_player garbled_player
       3 47 3
       47 3 0
       0 50 1)
  (run 83 50 unreliable_player jump_player step_player
       0 50 1
       47 3 0
       3 47 0))

(when ctl-c-ok?
  (run 123 10000 jump_player
       #:interrupt? #t
       '? '? '?)
  (when fail-ok?
    (for ([i 5])
      (run 123 10000 jump_player unreliable_player distracted_player
           #:interrupt? #t
           '? '? '?
           '? '? '?
           '? '? '?))))

(run 43 1000 step_player jump_player
     76 924 0
     924 76 0)

(cond
  [failed?
   (eprintf "Some test(s) failed\n")
   (exit 1)]
  [else
   (printf "All tests passed\n")])
