'use strict';
'require view.network.wireless_systemfix_v20261019_1 as current';

// LuCI factories return a constructor; preserve the canonical instance.
function CurrentModuleAlias() { return current; }
CurrentModuleAlias.prototype = Object.getPrototypeOf(current);
return CurrentModuleAlias;
