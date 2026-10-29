'use strict';
'require lg6851f.wireless_v20261023_53 as current';

// LuCI factories return a constructor; preserve the canonical instance.
function CurrentModuleAlias() { return current; }
CurrentModuleAlias.prototype = Object.getPrototypeOf(current);
return CurrentModuleAlias;
