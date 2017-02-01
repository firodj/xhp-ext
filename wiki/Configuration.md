XHP supports some configuration flags in `php.ini`. The most important is that you enable the extension via `extension=xhp.so`. After that’s done you can configure these other options:

**xhp.idx_expr** = *{"1 | 0}* (default: 0): This is a bonus feature of XHP which adds support for the `[]` operator on the return value of a function. If you enable this you will be able to write code like `foo()['bar']` without hitting syntax errors. This configuration is mainly around to fix a personal annoyance with PHP’s grammar, and has nothing to do with XHP’s core features.

**xhp.include_debug** = *{1 | 0}* (default: 1): You can disable XHP node annotations by turning this setting off. This may in some cases marginally increase performance of XHP-heavy code. It is highly recommended that you leave this on, as tracking errors in XHP applications is very difficult without annotations.
