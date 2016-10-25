<?php // xhp

class :div extends :x
{
    public function __toString()
    {
        $s = parent::__toString();
        debug_print_backtrace();
        throw new Exception("Where am I?");
        return $s;
    }
}
