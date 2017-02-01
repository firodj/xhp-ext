How It Works
============

This article is useful to hackers who are interested in how XHP works under the hood.

The XHP PHP extension which hooks into `compile_file` and `compile_string`. XHP will intercept calls to these functions and run all the code through a preprocessing step. The XHP compilation stage parses a superset of PHP and then returns code that PHP can understand. Therefore, all XHP really gives you is syntatic sugar via automatic metaprogramming.

Class Name Construction
=======================

Leading-colon class names are munged into `T_LABEL`’s that PHP can understand (since XHP accepts hyphens and colons in an element name, which are not valid in class names). The rule here is that `-` will become `_` and `:` will become `__`. The constructed class name will then be prepended with `xhp_`. So, `class :fb:thing-container {}` will become `class xhp_fb__thing_container {}` when PHP sees it. Note that this encoding should not be depended on, and may change in the future.

Element Instantiation
=====================

Element construction by the XHP XML syntax is rewritten to nested `new` operators. XHP assumes that every element is an object whose constructor takes two arguments. The first argument is a dictionary (array) of attributes, and the second argument is a list (array) of children. Entities in XHP are decoded at compile time to their raw equivalency; `&amp;` becomes `&` in the children list. These are then re-encoded back to entities at run-time.

`attribute`, `children`, and `category` Keywords
================================================

These keywords are rewritten to functions `__xhpAttributeDeclaration`, `__xhpChildrenDeclaration`, and `__xhpCategoryDeclaration`. Each of these functions returns some representation of the declaration. These can be fairly hairy, so you’re probably better off just looking at the PHP code which calls each of these functions.
