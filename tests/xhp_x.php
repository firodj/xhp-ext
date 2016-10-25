<?php // xhp
// Extremely simple implementation of XHP client library useful primarily for
// unit tests.
class :x {

  protected $tagName;

  public function __construct($attrs, $children) {
    $this->attrs = $attrs;
    $this->children = $children;
    $this->tagName = static::class2element(get_class($this));
  }

  public function __toString() {
    $head = '<'.$this->tagName;
    foreach ($this->attrs as $key => $val) {
      $head .= ' '.htmlspecialchars($key).'="'.htmlspecialchars($val, ENT_QUOTES).'"';
    }
    return $head.'>'.implode('', $this->children).'</' . $this->tagName . '>';
  }

  public static function class2element($class) {
    return str_replace(array('__', '_'), array(':', '-'), preg_replace('#^xhp_#i', '', $class));
  }
}
