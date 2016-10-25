<?php // xhp

class :a extends :x
{
    public function toString()
    {
        $this->children[] = <b>Bonus</b>;
        return parent::toString();
    }
}
