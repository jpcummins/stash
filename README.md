stash
=========

High performance mustache rendering engine written in C with extensions for Ruby.

For a list of implementations (other than Ruby and C) and tips, see
<http://mustache.github.com/>.

Usage
-----

Quick example:

    >> require 'stash'
    => true
    >> template = Template.new "Hello {{planet}}"
    >> template.render :planet => "World!"
    => "Hello World!"
