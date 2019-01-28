#lang racket

(module+ main
  (command-line
   #:args (so-file)
   (unless (file-exists? so-file)
     (error 'test "cannot find shared-library file ~a" so-file))
   (define c-file (path-replace-suffix so-file #".c"))
   (unless (file-exists? c-file)
     (error 'test "cannot find shared-library file ~a" c-file))
  (define so-dest "dest.so")
  (unless (system* "./enforce" so-file so-dest)
    (fprintf (current-error-port) "enforce failed\n")
    (exit 1))
   (define predictions (extract-predictions c-file))
   (printf "Test ~a\n" so-file) (flush-output)
   (for ([p (in-list predictions)])
     (try-call so-dest (prediction-function p) (prediction-arg p) (prediction-result p)))
   (when (zero? status)
     (printf "Passed\n"))
   (exit status)))

(define status 0)

(struct prediction (function arg result))

(define rx:prediction #rx"([a-zA-Z_0-9]+)[(](-?[0-9]+)[)] => (crash|-?[0-9]+)")

(define (extract-predictions c-file)
  (call-with-input-file
   c-file
   (lambda (i)
     (for*/list ([l (in-lines i)]
                 [m (in-value (regexp-match rx:prediction l))]
                 #:when m)
       (prediction (list-ref m 1) (list-ref m 2) (list-ref m 3))))))

(define (try-call so-dest function arg result)
  (define o (open-output-bytes))
  (define r
    (parameterize ([current-output-port o]
                   [current-error-port o])
      (system* "./call" so-dest function arg)))
  (cond
    [(equal? result "crash")
     (when r
       (bad! (format "expected crash for ~a(~a)" function arg)))]
    [else
     (define got (read-line (open-input-string (get-output-string o))))
     (unless (equal? result got)
       (define show-got (if (eof-object? got) "crash" got))
       (bad! (format "expected ~a(~a) => ~a, got ~a" function arg result show-got)))]))

(define (bad! msg)
  (fprintf (current-error-port) "~a\n" msg)
  (set! status 1))
