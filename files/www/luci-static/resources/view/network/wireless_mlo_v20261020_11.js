'use strict';
'require view.network.wireless_mlo_v20261020_12 as current';

// LuCI factories return a constructor; preserve the canonical instance.
function CurrentModuleAlias() { return current; }
CurrentModuleAlias.prototype = Object.getPrototypeOf(current);
return CurrentModuleAlias;
