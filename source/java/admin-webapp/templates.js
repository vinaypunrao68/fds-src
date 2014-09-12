angular.module('templates-main', ['scripts/account/account.html', 'scripts/activity/activity.html', 'scripts/admin/admin.html', 'scripts/admin/firebreak/firebreak.html', 'scripts/admin/profile/profile.html', 'scripts/directives/angular-fui/fui-checkbox/fui-checkbox.html', 'scripts/directives/angular-fui/fui-dropdown/fui-dropdown.html', 'scripts/directives/angular-fui/fui-slider/fui-slider.html', 'scripts/directives/widgets/linkbuttons/linkbuttons.html', 'scripts/directives/widgets/navitem/navitem.html', 'scripts/directives/widgets/priorityslider/priorityslider.html', 'scripts/directives/widgets/spinner/spinner.html', 'scripts/directives/widgets/variableslider/variableslider.html', 'scripts/inbox/inbox.html', 'scripts/main/main.html', 'scripts/status/status.html', 'scripts/system/addnode/addnode.html', 'scripts/system/system.html', 'scripts/users/users.html', 'scripts/users/users.tpl.html', 'scripts/volumes/create/nameType.html', 'scripts/volumes/create/qualityOfService.html', 'scripts/volumes/create/volumeCreate.html', 'scripts/volumes/volumes.html']);

angular.module("scripts/account/account.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/account/account.html",
    "<div ng-controller=\"accountController\" class=\"account-page\">\n" +
    "\n" +
    "    <div class=\"header top-label\">\n" +
    "        <span>Account Details</span>\n" +
    "    </div>\n" +
    "\n" +
    "    <div class=\"page-vertical-spacer\"></div>\n" +
    "\n" +
    "    <section class=\"block-section\">\n" +
    "\n" +
    "        <div class=\"section-header \">\n" +
    "            <span class=\"header\">Name</span>\n" +
    "        </div>\n" +
    "        <div>\n" +
    "            <span class=\"main-value\">Goldman</span>\n" +
    "            <a>Edit</a>\n" +
    "        </div>\n" +
    "    </section>\n" +
    "\n" +
    "    <div class=\"page-vertical-spacer\"></div>\n" +
    "\n" +
    "    <section class=\"block-section\">\n" +
    "\n" +
    "        <div class=\"section-header\">\n" +
    "            <span class=\"header\">Role</span>\n" +
    "        </div>\n" +
    "\n" +
    "        <div>\n" +
    "            <span class=\"main-value\">Tenant Admin</span>\n" +
    "            <a>Change Role</a>\n" +
    "        </div>\n" +
    "\n" +
    "        <div class=\"detail-value\">\n" +
    "            Tenant Admins can create modify and delete Tenant and App Developer users.  Additionally you can modify most system settings,create storage volumes and do everything else lower-level users can do.\n" +
    "        </div>\n" +
    "    </section>\n" +
    "\n" +
    "    <div class=\"page-vertical-spacer\"></div>\n" +
    "\n" +
    "    <section class=\"block-section\">\n" +
    "\n" +
    "        <div class=\"section-header\">\n" +
    "            <span class=\"header\">Email</span>\n" +
    "        </div>\n" +
    "\n" +
    "        <div>\n" +
    "            <input type=\"text\" ng-model=\"tempEmailAddress\" ng-show=\"email_changing\" class=\"form-control std-transition skinny\" placeholder=\"Email Address\" size=\"32\" style=\"width: 300px;\"/>\n" +
    "            <div style=\"display: flex;padding-top: 12px;\" ng-show=\"email_changing\" class=\"std-transition\">\n" +
    "                <button class=\"btn btn-primary\" style=\"margin-right: 4px;\" ng-click=\"emailAddress = tempEmailAddress;email_changing = false;\">Save</button>\n" +
    "                <button class=\"btn btn-secondary\" ng-click=\"tempEmailAddress = emailAddress;email_changing = false;\">Cancel</button>\n" +
    "            </div>\n" +
    "            <span class=\"detail-value\" style=\"font-size: 1.4em;\" ng-show=\"!email_changing\">{{emailAddress}}</span>\n" +
    "            <a ng-show=\"!email_changing\" ng-click=\"email_changing = true;\">Change Email</a>\n" +
    "        </div>\n" +
    "    </section>\n" +
    "\n" +
    "    <div class=\"page-vertical-spacer\"></div>\n" +
    "\n" +
    "    <section class=\"block-section\">\n" +
    "\n" +
    "        <div class=\"section-header\">\n" +
    "            <span class=\"header\">Password</span>\n" +
    "        </div>\n" +
    "\n" +
    "        <div ng-show=\"!password_changing\">\n" +
    "            <span class=\"detail-value\">********************</span>\n" +
    "            <a ng-click=\"password_changing=true;\">Change Password</a>\n" +
    "        </div>\n" +
    "\n" +
    "        <div ng-show=\"password_changing\">\n" +
    "<!--            <input type=\"password\" class=\"form-control skinny\" ng-model=\"password\" size=\"32\" placeholder=\"Old Password\" style=\"margin-bottom: 8px;width: 300px;\"/>-->\n" +
    "            <input type=\"password\" class=\"form-control skinny\" ng-model=\"newPassword\" size=\"32\" placeholder=\"New Password\" style=\"margin-bottom: 8px;width: 300px;\"/>\n" +
    "            <input type=\"password\" class=\"form-control skinny\" ng-model=\"confirmPassword\" size=\"32\" placeholder=\"Confirm Password\" style=\"margin-bottom: 8px;width: 300px;\"/>            \n" +
    "            <div class=\"error\" ng-show=\"changePasswordError !== false\">\n" +
    "                {{ changePasswordError }}\n" +
    "            </div>\n" +
    "            <div style=\"display: flex;padding-top: 12px;\" ng-show=\"password_changing\" class=\"std-transition\">\n" +
    "                <button class=\"btn btn-primary\" style=\"margin-right: 4px;\" ng-click=\"changePassword()\">Save</button>\n" +
    "                <button class=\"btn btn-secondary\" ng-click=\"password_changing = false;\">Cancel</button>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "\n" +
    "    </section>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/activity/activity.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/activity/activity.html",
    "<div ng-controller=\"activityController\">\n" +
    "\n" +
    "    <div class=\"coming-soon-parent\" ng-show=\"!isAllowed( 'System Management' )\">\n" +
    "        <div class=\"coming-soon-child\">\n" +
    "            <div style=\"\">\n" +
    "                <span style=\"color: white;\">Coming soon...</span>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "    \n" +
    "    <div class=\"header top-label\">\n" +
    "        <span>System Activity</span>\n" +
    "    </div>\n" +
    "\n" +
    "    <div class=\"page-vertical-spacer\"></div>\n" +
    "\n" +
    "    <div class=\"header top-label\">\n" +
    "            <input type=\"text\" class=\"form-control skinny\" placeholder=\"Search...\" ng-model=\"searchQuery\" style=\"width: 300px;\">\n" +
    "        <div>\n" +
    "            <table class=\"default\" style=\"margin-top: 8px;\">\n" +
    "                <tbody>\n" +
    "                    <tr ng-repeat=\"activity in activities | filter:searchQuery\">\n" +
    "                        <td style=\"text-align: left;\">{{activity.type}}</td>\n" +
    "                        <td>{{activity.message}}</td>\n" +
    "                        <td style=\"text-align: right;\">{{activity.time}}</td>\n" +
    "                    </tr>\n" +
    "                </tbody>\n" +
    "            </table>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/admin/admin.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/admin/admin.html",
    "<div ng-controller=\"adminController\" style=\"padding-left: 15px\" class=\"admin\">\n" +
    "\n" +
    "    <div class=\"header light-print admin-header\">\n" +
    "        <span>Admin Dashboard</span>\n" +
    "    </div>\n" +
    "\n" +
    "    <section class=\"form-section\">\n" +
    "        <div>\n" +
    "            <span class=\"light-print\">System Version</span>\n" +
    "        </div>\n" +
    "\n" +
    "        <div class=\"version-text\">\n" +
    "            v1.05\n" +
    "        </div>\n" +
    "        <div class=\"upgrade-panel\" ng-show=\"isAllowed( 'Tenant Management' )\">\n" +
    "            <div>\n" +
    "                <span>Update available: v1.06</span>\n" +
    "                <span class=\"primary-color\">Release notes</span>\n" +
    "            </div>\n" +
    "            <div class=\"button-panel\">\n" +
    "                <span>\n" +
    "                    <button class=\"btn btn-primary\">Update Now</button>\n" +
    "                </span>\n" +
    "                <span>or</span>\n" +
    "                <span class=\"primary-color\">Rollback</span>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "\n" +
    "        <div class=\"admin-header\"></div>\n" +
    "    </section>\n" +
    "\n" +
    "    <section class=\"form-section\">\n" +
    "        <div>\n" +
    "            <span class=\"light-print\">Data Connectors</span>\n" +
    "        </div>\n" +
    "\n" +
    "        <div class=\"volume-data-type-panel open\">\n" +
    "            <table class=\"default volumes\">\n" +
    "                <thead>\n" +
    "                    <th>Data Type</th>\n" +
    "                    <th>API</th>\n" +
    "                    <th>Default Capacity</th>\n" +
    "                    <th></th>\n" +
    "                </thead>\n" +
    "                <tbody id=\"data-connectors\">\n" +
    "                    <tr ng-repeat=\"connector in connectors\" ng-click=\"setSelected( connector );\">\n" +
    "                        <td>{{connector.type}}</td>\n" +
    "                        <td>{{connector.api}}</td>\n" +
    "                        <td>\n" +
    "                            <span class=\"connector-attributes\" ng-show=\"connector.attributes && connector.attributes.size\">\n" +
    "                                <button class=\"btn btn-primary skinny\" ng-click=\"editConnector( connector )\" ng-show=\"editingConnector != connector\">\n" +
    "                                    {{connector.attributes.size}}\n" +
    "                                    {{connector.attributes.unit}}\n" +
    "                                </button>\n" +
    "                                <div ng-show=\"editingConnector == connector\">\n" +
    "                                    <div class=\"inline vertical-middle\">\n" +
    "                                        <spinner value=\"$parent._selectedSize\" min=\"1\" max=\"999\" step=\"1\" class=\"skinny\"></spinner>\n" +
    "                                    </div>\n" +
    "                                    <div class=\"inline\">\n" +
    "                                        <fui-dropdown skinny=\"true\" background-color=\"#2486F8\" items=\"sizes\" selected=\"$parent._selectedUnit\"></fui-dropdown>\n" +
    "                                    </div>\n" +
    "                                    <button class=\"btn btn-primary skinny\" ng-click=\"saveConnectorChanges( connector )\">Save</button>\n" +
    "                                    <button class=\"btn btn-secondary skinny\" ng-click=\"stopEditing()\">Cancel</button>\n" +
    "                                </div>\n" +
    "                            </span>\n" +
    "                        </td>\n" +
    "                        <td class=\"checkmark\">\n" +
    "                            <span class=\"fui-arrow-right light-print\"></span>\n" +
    "                        </td>\n" +
    "                    </tr>\n" +
    "                </tbody>\n" +
    "            </table>\n" +
    "        </div>\n" +
    "    </section>\n" +
    "\n" +
    "    <section class=\"form-section log-and-alert-panel\" ng-show=\"isAllowed( 'Tenant Management' )\">\n" +
    "        <div class=\"spacer\">\n" +
    "            <span class=\"light-print\">Log and Alert Settings</span>\n" +
    "        </div>\n" +
    "        <div>\n" +
    "            <fui-dropdown skinny=\"true\" items=\"levels\" selected=\"alertLevel\" color=\"black\" background=\"none\" border=\"gray\"></fui-dropdown>\n" +
    "        </div>\n" +
    "    </section>\n" +
    "\n" +
    "\n" +
    "    <!-- firebreak settings -->\n" +
    "<!--    <div ng-include=\"'scripts/admin/firebreak/firebreak.html'\"></div>-->\n" +
    "\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/admin/firebreak/firebreak.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/admin/firebreak/firebreak.html",
    "<section class=\"form-section\" ng-controller=\"firebreakController\">\n" +
    "    <div class=\"header\">\n" +
    "        <span>Firebreak Settings</span>\n" +
    "    </div>\n" +
    "    <div class=\"collapse-form-edit\" ng-class=\"{open: !editing}\">\n" +
    "        <div class=\"form-group inline\">\n" +
    "            <span class=\"value\">{{policies.length}}</span>\n" +
    "            <span>policies configured</span>\n" +
    "        </div>\n" +
    "        <div class=\"inline icon-container\">\n" +
    "            <span class=\"icon fui-new\" ng-click=\"editing = true;\" title=\"Edit settings\"></span>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "\n" +
    "    <div class=\"collapse-form-edit\" ng-class=\"{open: editing}\">\n" +
    "\n" +
    "        <div style=\"position: relative;height: 190px;transition: linear 200ms\" ng-style=\"{left: creating ? '-100%' : '0px'}\">\n" +
    "            <!-- actual table -->\n" +
    "            <div class=\"table-container\" style=\"position: absolute;left: 0px;top: 0px;width: 100%;\">\n" +
    "                <div class=\"header\">\n" +
    "                    <a class=\"pull-right\" ng-click=\"creating = true;\">Add Policy</a>\n" +
    "                </div>\n" +
    "                <table class=\"default volumes clickable\" style=\"margin-bottom: 48px;margin-left: 0px;\">\n" +
    "                    <thead>\n" +
    "                        <th>Volume Priority</th>\n" +
    "                        <th>Violations</th>\n" +
    "                        <th>Time Period</th>\n" +
    "                        <th>Action</th>\n" +
    "                        <th></th>\n" +
    "                    </thead>\n" +
    "                    <tbody>\n" +
    "                        <tr class=\"disabled\" ng-show=\"!pokicies.length || policies.length === 0\">\n" +
    "                            <td colspan=\"100%\">\n" +
    "                                There are no policies configured.\n" +
    "                            </td>\n" +
    "                        </tr>\n" +
    "                    </tbody>\n" +
    "                </table>\n" +
    "                <div class=\"button-group\">\n" +
    "                    <button class=\"btn btn-secondary\" ng-click=\"editing = false;\">Cancel</button>\n" +
    "                    <button class=\"btn btn-primary\">Save</button>\n" +
    "                </div>\n" +
    "            </div>\n" +
    "            <div style=\"position: absolute;width: 100%;left: 100%;\">\n" +
    "                <button class=\"btn btn-primary\" ng-click=\"creating = false;\">Done</button>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "\n" +
    "</section>\n" +
    "");
}]);

angular.module("scripts/admin/profile/profile.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/admin/profile/profile.html",
    "<section class=\"form-section\" ng-controller=\"profileController\">\n" +
    "    <div class=\"header\">\n" +
    "        <span>Profile</span>\n" +
    "    </div>\n" +
    "    <div class=\"collapse-form-edit\" ng-class=\"{open: !editing}\">\n" +
    "        <div class=\"form-group inline\">\n" +
    "            <span>User:</span>\n" +
    "            <span class=\"value\">{{user.name}}</span>\n" +
    "        </div>\n" +
    "        <div class=\"inline icon-container\">\n" +
    "            <span class=\"icon fui-new\" ng-click=\"edit();\" title=\"Edit settings\"></span>\n" +
    "        </div>\n" +
    "        <a ng-click=\"changepassword = true;\">Change Password</a>\n" +
    "    </div>\n" +
    "\n" +
    "    <!-- change password form -->\n" +
    "    <div class=\"collapse-form-edit\" ng-class=\"{open: changepassword}\">\n" +
    "        <div class=\"header\">\n" +
    "            <span>Current Password</span>\n" +
    "        </div>\n" +
    "        <div class=\"form-group\">\n" +
    "            <input type=\"password\" class=\"form-control\" size=\"32\" placeholder=\"\" ng-model=\"_password\">\n" +
    "        </div>\n" +
    "        <div class=\"header\">\n" +
    "            <span>New Password</span>\n" +
    "        </div>\n" +
    "        <div class=\"form-group\">\n" +
    "            <input type=\"password\" class=\"form-control\" size=\"32\" placeholder=\"\" ng-model=\"_new_password\">\n" +
    "        </div>\n" +
    "        <div class=\"header\">\n" +
    "            <span>Confirm Password</span>\n" +
    "        </div>\n" +
    "        <div class=\"form-group\">\n" +
    "            <input type=\"password\" class=\"form-control\" size=\"32\" placeholder=\"\" ng-model=\"_confirm_password\">\n" +
    "        </div>\n" +
    "        <div class=\"pull-left\">\n" +
    "            <button class=\"btn btn-secondary\" ng-click=\"changepassword = false;\">Cancel</button>\n" +
    "            <button class=\"btn btn-primary\" ng-click=\"changePassword()\">Save</button>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "\n" +
    "    <!-- User info editing form -->\n" +
    "    <div class=\"collapse-form-edit\" ng-class=\"{open: editing}\">\n" +
    "        <section>\n" +
    "            <div class=\"header\">\n" +
    "                <span>Name</span>\n" +
    "            </div>\n" +
    "            <div class=\"form-group\">\n" +
    "                <input type=\"text\" class=\"form-control\" size=\"32\" placeholder=\"Full name\" ng-model=\"_username\">\n" +
    "                <div class=\"clear-icon\" ng-click=\"_username = ''\">\n" +
    "                </div>\n" +
    "            </div>\n" +
    "            <div class=\"header\">\n" +
    "                <span>Email Address</span>\n" +
    "            </div>\n" +
    "            <div class=\"form-group\">\n" +
    "                <input type=\"text\" class=\"form-control\" size=\"32\" placeholder=\"Email address\" ng-model=\"_email\">\n" +
    "                <div class=\"clear-icon\" ng-click=\"_email = ''\"></div>\n" +
    "            </div>\n" +
    "            <div class=\"pull-left\">\n" +
    "                <button class=\"btn btn-secondary\" ng-click=\"editing = false;\">Cancel</button>\n" +
    "                <button class=\"btn btn-primary\" ng-click=\"save()\">Save</button>\n" +
    "            </div>\n" +
    "        </section>\n" +
    "    </div>\n" +
    "</section>\n" +
    "");
}]);

angular.module("scripts/directives/angular-fui/fui-checkbox/fui-checkbox.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/directives/angular-fui/fui-checkbox/fui-checkbox.html",
    "<label class=\"checkbox\" ng-class=\"{checked: checked,unchecked: !checked}\" for=\"fuiCheck\">\n" +
    "    <span class=\"icons\">\n" +
    "        <span class=\"first-icon fui-checkbox-unchecked\"></span>\n" +
    "        <span class=\"second-icon fui-checkbox-checked\" ng-click=\"checked = !checked;\"></span>\n" +
    "    </span>\n" +
    "    <input type=\"checkbox\" value=\"\" id=\"fuiCheck\" />\n" +
    "    {{label}}\n" +
    "</label>\n" +
    "");
}]);

angular.module("scripts/directives/angular-fui/fui-dropdown/fui-dropdown.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/directives/angular-fui/fui-dropdown/fui-dropdown.html",
    "<div>\n" +
    "    <div class=\"dropdown\" ng-class=\"{open: open === true, 'dropdown-no-background' : background === 'none'}\" ng-click=\"openList( $event )\">\n" +
    "        <button class=\"btn btn-primary dropdown-toggle\" ng-class=\"{'dropdown-no-background' : background === 'none', skinny: skinny, 'dropdown-gray-border': border === 'gray' }\" ng-style=\"{'background-color': backgroundColor, color: color }\">\n" +
    "            <span ng-class=\"{'pull-left': type === 'select'}\">{{currentLabel}}</span>\n" +
    "            <span class=\"caret\" ng-style=\"{color: color, 'border-top-color': color}\"></span>\n" +
    "        </button>\n" +
    "        <span class=\"dropdown-arrow dropdown-arrow-inverse\"></span>\n" +
    "        <ul class=\"dropdown-menu dropdown-inverse\" ng-style=\"{ left: leftShift }\">\n" +
    "            <li ng-repeat=\"item in items\">\n" +
    "                <a ng-click=\"selectItem( item );\" ng-show=\"item.name !== '-'\">{{item.name}}</a>\n" +
    "                <div ng-show=\"item.name === '-'\" class=\"divider\"></div>\n" +
    "            </li>\n" +
    "        </ul>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/directives/angular-fui/fui-slider/fui-slider.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/directives/angular-fui/fui-slider/fui-slider.html",
    "<div class=\"ui-slider ui-slider-horizontal ui-widget ui-widget-content ui-corner-all\" ng-click=\"adjustSlider( getClickX( $event ) )\">\n" +
    "    <div class=\"ui-slider-segment\" ng-repeat=\"segment in segments\" ng-style=\"{'margin-left': segment.px + 'px'}\"></div>\n" +
    "    <div class=\"ui-slider-range ui-widget-header ui-corner-all ui-slider-range-min\" ng-style=\"{width: position + 'px'}\"></div>\n" +
    "    <a class=\"ui-slider-handle ui-state-default ui-corner-all\"\n" +
    "       ng-mousedown=\"grabbed = true;\" ng-style=\"{left: position + 'px'}\"></a>\n" +
    "    <div class=\"ui-slider-labels\">\n" +
    "        <span class=\"pull-left light-print\">{{minLabel}}</span>\n" +
    "        <span class=\"pull-right light-print\">{{maxLabel}}</span>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/directives/widgets/linkbuttons/linkbuttons.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/directives/widgets/linkbuttons/linkbuttons.html",
    "<div class=\"linkbutton-parent\">\n" +
    "\n" +
    "    <div class=\"pull-left linkbutton-item\" ng-repeat=\"item in items\" ng-click=\"item.callback();\">\n" +
    "        <span ng-class=\"{selected: item.selected}\">{{item.label}}</span>\n" +
    "        <span ng-show=\"item.notifications && item.notifications > 0\">({{item.notifications}})</span>\n" +
    "        <span class=\"linkbutton-bar\" ng-show=\"!$last\">|</span>\n" +
    "    </div>\n" +
    "\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/directives/widgets/navitem/navitem.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/directives/widgets/navitem/navitem.html",
    "<div class=\"navitem-parent\" ng-class=\"{'navitem-selected': selected }\">\n" +
    "    <div class=\"nav-icon\" ng-style=\"{'background-image': 'url(' + icon + ')'}\"></div>\n" +
    "    <div class=\"nav-text\">{{label}}</div>\n" +
    "    <div class=\"navitem-badge-parent\">\n" +
    "        <div class=\"navitem-badge\" ng-show=\"notifications && notifications > 0\">{{notifications}}\n" +
    "        </div>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/directives/widgets/priorityslider/priorityslider.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/directives/widgets/priorityslider/priorityslider.html",
    "<div class=\"slider base\">\n" +
    "\n" +
    "    <div class=\"slider strikethrough\"></div>\n" +
    "\n" +
    "    <div class=\"slider container\">\n" +
    "        <div class=\"slider item\" ng-repeat=\"p in priorities\" ng-style=\"{width: widthPer}\" ng-class=\"{last: $last}\" ng-click=\"setPriority( p )\">\n" +
    "            <div class=\"slider bubble\" ng-class=\"{current: p == priority}\">\n" +
    "                <span>{{p}}</span>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "    <div class=\"pull-left clearfix\" style=\"width: 100%;\">\n" +
    "        <label class=\"slider pull-left low\">\n" +
    "            Lowest\n" +
    "        </label>\n" +
    "        <label class=\"slider pull-right high\">\n" +
    "            Highest\n" +
    "        </label>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/directives/widgets/spinner/spinner.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/directives/widgets/spinner/spinner.html",
    "<div class=\"spinner-parent\">\n" +
    "    <input type=\"text\" ng-model=\"value\" maxlength=\"{{maxlength}}\" size=\"{{maxlength}}\"/>\n" +
    "    <div class=\"button-parent\">\n" +
    "        <div class=\"button-base increment\" draggable=\"false\" ng-class=\"{pressed: upPressed}\" ng-mousedown=\"incrementDown()\" ng-mouseup=\"incrementUp()\" ng-mouseleave=\"incrementUp()\" ng-mousemove=\"incrementUp()\" ng-click=\"increment()\">\n" +
    "            <span class=\"caret\"></span>\n" +
    "        </div>\n" +
    "        <div class=\"button-base decrement\" draggable=\"false\" ng-class=\"{pressed: downPressed}\" ng-mousedown=\"decrementDown()\" ng-mouseup=\"decrementUp()\" ng-mouseleave=\"decrementUp()\" ng-mousemove=\"decrementUp()\" ng-click=\"decrement()\">\n" +
    "            <span class=\"caret\"></span>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/directives/widgets/variableslider/variableslider.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/directives/widgets/variableslider/variableslider.html",
    "<div class=\"slider base\">\n" +
    "    <div class=\"slider strikethrough\"></div>\n" +
    "\n" +
    "    <div class=\"slider items variable\">\n" +
    "        <div class=\"slider variable bubble\" ng-repeat=\"s in selections\" ng-click=\"setSelection( s )\" ng-class=\"{current: s == selection}\">\n" +
    "            <span>{{s}}</span>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "    <div class=\"pull-left clearfix\" style=\"width: 100%;\">\n" +
    "        <label class=\"slider pull-left low\">\n" +
    "            {{ description }}\n" +
    "        </label>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/inbox/inbox.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/inbox/inbox.html",
    "<div ng-controller=\"inboxController\">\n" +
    "\n" +
    "    <div class=\"coming-soon-parent\" ng-show=\"!isAllowed( 'System Management' )\">\n" +
    "        <div class=\"coming-soon-child\">\n" +
    "            <div style=\"\">\n" +
    "                <span style=\"color: white;\">Coming soon...</span>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "    \n" +
    "    <div class=\"header top-label\">\n" +
    "        <div style=\"position:relative;\">\n" +
    "            <div class=\"pull-left\">Messages</div>\n" +
    "            <div class=\"pull-right\">\n" +
    "                <linkbuttons items=\"items\"></linkbuttons>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "\n" +
    "        <table class=\"default\">\n" +
    "            <thead>\n" +
    "                <th style=\"width: 24px;vertical-align: middle;\"><input type=\"checkbox\"/></th>\n" +
    "                <th>Type</th>\n" +
    "                <th>Message</th>\n" +
    "                <th>Received</th>\n" +
    "            </thead>\n" +
    "            <tbody>\n" +
    "                <tr ng-repeat=\"message in messages\">\n" +
    "                    <td style=\"width: 24px;vertical-align: middle;\"><input type=\"checkbox\"/></td>\n" +
    "                    <td>{{message.type}}</td>\n" +
    "                    <td>{{message.message}}</td>\n" +
    "                    <td>{{message.time}}</td>\n" +
    "                </tr>\n" +
    "            </tbody>\n" +
    "        </table>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/main/main.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/main/main.html",
    "<div ng-controller=\"mainController\" class=\"main\">\n" +
    "    <div class=\"login\" ng-show=\"!validAuth\">\n" +
    "\n" +
    "        <div class=\"login-container\">\n" +
    "            <div class=\"logo\"></div>\n" +
    "            <form>\n" +
    "<!--\n" +
    "                <div class=\"btn-group select\">\n" +
    "                    <fui-dropdown selected=\"domain\" default-label=\"Domain\" items=\"domains\"></fui-dropdown>\n" +
    "                </div>\n" +
    "-->\n" +
    "\n" +
    "                <div class=\"form-group\">\n" +
    "                    <input type=\"text\" class=\"form-control\" name=\"username\" autofocus autocomplete=\"off\" autocapitalize=\"off\" ng-model=\"username\" placeholder=\"Email\"/>\n" +
    "                    <span class=\"input-icon fui-mail\"></span>\n" +
    "                </div>\n" +
    "                <div class=\"form-group\">\n" +
    "                    <input type=\"password\" class=\"form-control\" name=\"password\" autocomplete=\"off\" ng-model=\"password\" placeholder=\"Password\" ng-keyup=\"keyEntered( $event );\">\n" +
    "                    <span class=\"input-icon fui-lock\"></span>\n" +
    "                </div>\n" +
    "\n" +
    "                <div class=\"checkbox-parent\">\n" +
    "                    <fui-checkbox label=\"Remember me\" checked=\"rememberChecked\"></fui-checkbox>\n" +
    "                </div>\n" +
    "\n" +
    "                <div class=\"form-actions\">\n" +
    "                    <button id=\"login.submit\" class=\"btn btn-primary\" ng-click=\"login()\">\n" +
    "                        Login\n" +
    "                    </button>\n" +
    "                </div>\n" +
    "\n" +
    "                <div class=\"alert alert-info\" ng-show=\"error\">\n" +
    "                    {{ error }}\n" +
    "                </div>\n" +
    "\n" +
    "                <div class=\"lost-password\">\n" +
    "                    Lost your password?\n" +
    "                </div>\n" +
    "            </form>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "\n" +
    "    <div id=\"main.content\" ng-show=\"validAuth\" style=\"color: white;height: 100%;overflow: hidden;\">\n" +
    "\n" +
    "        <div class=\"header-parent\">\n" +
    "            <div class=\"header-image\">\n" +
    "            </div>\n" +
    "            <div style=\"float: right;display: flex;\">\n" +
    "                <fui-dropdown id=\"main.usermenu\" selected=\"loggedInUser\" items=\"menuItems\" default-label=\"loggedInUser\" background=\"none\" type=\"action\" callback=\"itemSelected\"></fui-dropdown>\n" +
    "\n" +
    "            </div>\n" +
    "        </div>\n" +
    "        <div class=\"nav-container\" style=\"position: relative;\">\n" +
    "            <div id=\"wrap\" class=\"main-container\" ui-view>\n" +
    "\n" +
    "            </div>\n" +
    "            <div class=\"nav-parent\">\n" +
    "                <div>\n" +
    "                    <nav id=\"nav\">\n" +
    "                        <ul>\n" +
    "                            <li ng-repeat=\"view in views\">\n" +
    "                                <navitem id=\"main.{{view.id}}\" label=\"{{view.text}}\" icon-class=\"{{view.iconClass}}\" selected=\"view.selected\" ng-click=\"navigate( view );\" notifications=\"view.notifications\" ng-show=\"isAllowed( view.permission );\"></navitem>\n" +
    "                            </li>\n" +
    "                        </ul>\n" +
    "                    </nav>\n" +
    "                </div>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/status/status.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/status/status.html",
    "<div ng-controller=\"statusController\" style=\"position: relative;\">\n" +
    "\n" +
    "    <div class=\"coming-soon-parent\" ng-show=\"!isAllowed( 'System Management' )\">\n" +
    "        <div class=\"coming-soon-child\">\n" +
    "            <div style=\"\">\n" +
    "                <span style=\"color: white;\">Coming soon...</span>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "    \n" +
    "    <div class=\"tile-container\">\n" +
    "        <div class=\"tile-pair\">\n" +
    "            <div class=\"tile-parent\">\n" +
    "                <div class=\"system-tile tile\"></div>\n" +
    "            </div>\n" +
    "            <div class=\"tile-parent\">\n" +
    "                <div class=\"capacity-tile tile\"></div>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "        <div class=\"tile-pair\">\n" +
    "            <div class=\"tile-parent\">\n" +
    "                <div class=\"performance-tile tile\"></div>\n" +
    "            </div>\n" +
    "            <div class=\"tile-parent\">\n" +
    "                <div class=\"firebreak-tile tile\"></div>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "\n" +
    "    <div class=\"page-vertical-spacer\"></div>\n" +
    "    <div class=\"page-vertical-spacer\"></div>\n" +
    "\n" +
    "    <div class=\"header top-label\">\n" +
    "        <span class=\"header\" style=\"margin-bottom: 12px;\">Activity</span>\n" +
    "\n" +
    "        <table class=\"default\">\n" +
    "            <tbody>\n" +
    "                <tr ng-repeat=\"activity in activities\">\n" +
    "                    <td style=\"text-align: left;\">{{activity.type}}</td>\n" +
    "                    <td>{{activity.message}}</td>\n" +
    "                    <td style=\"text-align: right;\">{{activity.time}}</td>\n" +
    "                </tr>\n" +
    "            </tbody>\n" +
    "        </table>\n" +
    "    </div>\n" +
    "    \n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/system/addnode/addnode.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/system/addnode/addnode.html",
    "<div class=\"dialog-page\" ng-class=\"{shown: creating, hideit: !creating}\" ng-controller=\"addNodeController\">\n" +
    "\n" +
    "    <div class=\"modal-header\">\n" +
    "        <span>Add Node</span>\n" +
    "    </div>\n" +
    "\n" +
    "    <table class=\"default clickable\">\n" +
    "        <thead>\n" +
    "            <th></th>\n" +
    "            <th>Node Name</th>\n" +
    "            <th>Site</th>\n" +
    "            <th>Local Domain</th>\n" +
    "        </thead>\n" +
    "        <tbody>\n" +
    "            <tr ng-repeat=\"detachedNode in detachedNodes\">\n" +
    "                <td width=\"32\">\n" +
    "                    <fui-checkbox checked=\"detachedNode.addall\"></fui-checkbox>\n" +
    "                </td>\n" +
    "                <td>\n" +
    "                    {{detachedNode.node_name}}\n" +
    "                </td>\n" +
    "                <td>\n" +
    "                    {{detachedNode.site}}\n" +
    "                </td>\n" +
    "                <td>\n" +
    "                    {{detachedNode.local_domain}}\n" +
    "                </td>\n" +
    "            </tr>\n" +
    "            <tr class=\"disabled\" ng-show=\"!detachedNodes.length || detachedNodes.length === 0\">\n" +
    "                <td colspan=\"100%\">\n" +
    "                    No detached nodes were discovered.\n" +
    "                </td>\n" +
    "            </tr>\n" +
    "        </tbody>\n" +
    "    </table>\n" +
    "    <div class=\"pull-left\">\n" +
    "        <button class=\"btn btn-primary\" ng-click=\"addNodes()\">Add Nodes</button>\n" +
    "        <button class=\"btn btn-secondary\" ng-click=\"cancel()\">Cancel</button>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/system/system.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/system/system.html",
    "<div ng-controller=\"systemController\" class=\"page-slider-parent\" ng-style=\"{left: creating ? '-100%' : '0px'}\">\n" +
    "<!--\n" +
    "    <div class=\"col-xs-12\">\n" +
    "        <div class=\"header\">\n" +
    "            <span>Global Domain</span>\n" +
    "        </div>\n" +
    "        <div class=\"panel header\">\n" +
    "            <span>{{ domain }}</span>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "-->\n" +
    "    <div class=\"col-xs-12 node-list front-page\">\n" +
    "        <div class=\"header\">\n" +
    "            <span class=\"light-print\">Nodes</span>\n" +
    "            <a data-href=\"/nodes/new\" class=\"pull-right\" ng-click=\"creating = true;\">Add a Node</a>\n" +
    "        </div>\n" +
    "        <div class=\"nodes-container table-container\">\n" +
    "            <table class=\"default\">\n" +
    "                <thead>\n" +
    "                    <th></th>\n" +
    "                    <th>Node Name</th>\n" +
    "                    <th>Site</th>\n" +
    "                    <th>Local Domain</th>\n" +
    "<!--\n" +
    "                    <th>HW</th>\n" +
    "                    <th>OM</th>\n" +
    "                    <th>SM</th>\n" +
    "                    <th>DM</th>\n" +
    "                    <th>AM</th>\n" +
    "                    <th></th>\n" +
    "-->\n" +
    "                </thead>\n" +
    "                <tbody>\n" +
    "                    <tr ng-repeat-start=\"node in nodes\" class=\"node-item\" ng-click=\"showStatusDetails( node )\">\n" +
    "                        <td><span class=\"fui-arrow-right\" ng-class=\"{ open: node.showDetails }\"></span><span class=\"{{getStatus( node )}}\"></span></td>\n" +
    "                        <td>{{node.node_name}}</td>\n" +
    "                        <td>{{node.site}}</td>\n" +
    "                        <td>{{node.domain}}</td>\n" +
    "<!--\n" +
    "                        <td><span class=\"{{getIcon( node.hw )}}\"></span></td>\n" +
    "                        <td><span class=\"{{getIcon( node.om )}}\"></span></td>\n" +
    "                        <td><span class=\"{{getIcon( node.sm )}}\"></span></td>\n" +
    "                        <td><span class=\"{{getIcon( node.dm )}}\"></span></td>\n" +
    "                        <td><span class=\"{{getIcon( node.am )}}\"></span></td>\n" +
    "-->\n" +
    "<!--                        <td><span class=\"icon-right small muted pull-right\"></i></td>-->\n" +
    "                    </tr>\n" +
    "                    <tr ng-repeat-end class=\"node-status-row\" ng-class=\"{closed: !node.showDetails }\">\n" +
    "                        <td colspan=\"100%\">\n" +
    "                            <div class=\"node-status-details\" ng-class=\"{ open: node.showDetails }\">\n" +
    "                                <div class=\"node-columns\">\n" +
    "                                    <div>\n" +
    "                                        <span class=\"{{getIcon( node.hw )}}\"></span>\n" +
    "                                        <span>Hardware</span>\n" +
    "                                    </div>\n" +
    "                                    <div>\n" +
    "                                        <span class=\"{{getIcon( node.om )}}\"></span>\n" +
    "                                        <span>Orchestration Manager</span>\n" +
    "                                    </div>\n" +
    "                                    <div>\n" +
    "                                        <span class=\"{{getIcon( node.sm )}}\"></span>\n" +
    "                                        <span>Storage Manager</span>\n" +
    "                                    </div>\n" +
    "                                    <div>\n" +
    "                                        <span class=\"{{ getIcon( node.dm )}}\"></span>\n" +
    "                                        <span>Data Manager</span>\n" +
    "                                    </div>\n" +
    "                                    <div>\n" +
    "                                        <span class=\"{{ getIcon( node.am ) }}\"></span>\n" +
    "                                        <span>Application Manager</span>\n" +
    "                                    </div>\n" +
    "                                </div>\n" +
    "                                <div class=\"node-columns active-status\">\n" +
    "                                    <div>\n" +
    "                                        {{ activeText( node.hw ) }}\n" +
    "                                    </div>\n" +
    "                                    <div>\n" +
    "                                        {{ activeText( node.om ) }}\n" +
    "                                    </div>\n" +
    "                                    <div>\n" +
    "                                        {{ activeText( node.sm ) }}\n" +
    "                                    </div>\n" +
    "                                    <div>\n" +
    "                                        {{ activeText( node.dm ) }}\n" +
    "                                    </div>\n" +
    "                                    <div>\n" +
    "                                        {{ activeText( node.am ) }}\n" +
    "                                    </div>\n" +
    "                                </div>\n" +
    "                            </div>\n" +
    "                        </td>\n" +
    "                    </tr>\n" +
    "                </tbody>\n" +
    "            </table>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "\n" +
    "    <div ng-include=\"'scripts/system/addnode/addnode.html'\"></div>\n" +
    "<!--\n" +
    "    <div class=\"row\">\n" +
    "        <div class=\"col-xs-12 legend\">\n" +
    "            <div class=\"header\">\n" +
    "                <span>Legend</span>\n" +
    "            </div>\n" +
    "            <div class=\"col-xs-5 content\">\n" +
    "                <span class=\"list\">HW = Hardware</span>\n" +
    "                <span class=\"list\">OM = Orchestration Manager</span>\n" +
    "                <span class=\"list\">SM = Storage Manager</span>\n" +
    "                <span class=\"list\">DM = Data Manager</span>\n" +
    "                <span class=\"list\">AM = Application Manager</span>\n" +
    "            </div>\n" +
    "            <div class=\"col-xs-5 content\">\n" +
    "                <span class=\"list\"><span class=\"fui-check-inverted-2\"></span>Active</span>\n" +
    "                <span class=\"list\"><span class=\"fui-radio-unchecked\"></span>Inactive</span>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "-->\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/users/users.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/users/users.html",
    "<div ng-controller=\"userController\">\n" +
    "\n" +
    "    <div class=\"coming-soon-parent\" ng-show=\"!isAllowed( 'System Management' )\">\n" +
    "        <div class=\"coming-soon-child\">\n" +
    "            <div style=\"\">\n" +
    "                <span style=\"color: white;\">Coming soon...</span>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </div>    \n" +
    "    \n" +
    "    <div class=\"header top-label\">\n" +
    "\n" +
    "        <div>\n" +
    "            <span>Users</span>\n" +
    "        </div>\n" +
    "\n" +
    "        <div style=\"position: relative;width: 100%;padding-top: 12px;\" class=\"clearfix\">\n" +
    "            <div class=\"pull-left\" style=\"width: 300px;\">\n" +
    "                <input type=\"text\" class=\"form-control skinny\" placeholder=\"Search Users\" ng-model=\"userQuery\"/>\n" +
    "            </div>\n" +
    "            <div class=\"pull-right\">\n" +
    "                <a>Add User(s)</a>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "\n" +
    "        <div style=\"display: flex;padding-top: 12px;\">\n" +
    "            <div style=\"padding-right: 8px;\">\n" +
    "                <fui-dropdown skinny=\"true\" type=\"action\" items=\"actionItems\" default-label=\"actionLabel\" callback=\"actionSelected\" background=\"none\" color=\"black\" border=\"gray\"></fui-dropdown>\n" +
    "            </div>\n" +
    "            <div style=\"font-size: 0.6em;\">\n" +
    "                <div>for 0</div>\n" +
    "                <div>selected users</div>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "\n" +
    "        <table class=\"default\">\n" +
    "            <thead>\n" +
    "                <th><input type=\"checkbox\" /></th>\n" +
    "                <th>Name</th>\n" +
    "                <th>Tenant</th>\n" +
    "                <th>Role</th>\n" +
    "                <th>Access</th>\n" +
    "                <th>Last Activity</th>\n" +
    "            </thead>\n" +
    "            <tboday>\n" +
    "                <tr ng-repeat=\"user in users | filter:userQuery\">\n" +
    "                    <td><input type=\"checkbox\" /></td>\n" +
    "                    <td>{{user.firstname}} {{user.lastname}}</td>\n" +
    "                    <td>{{user.tenant}}</td>\n" +
    "                    <td>{{user.role}}</td>\n" +
    "                    <td>{{user.access}}</td>\n" +
    "                    <td>{{user.last_activity}}</td>\n" +
    "                </tr>\n" +
    "            </tboday>\n" +
    "        </table>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/users/users.tpl.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/users/users.tpl.html",
    "<div ng-controller=\"userController\">\n" +
    "\n" +
    "    <div class=\"header top-label\">\n" +
    "\n" +
    "        <div>\n" +
    "            <span>Users</span>\n" +
    "        </div>\n" +
    "\n" +
    "        <div style=\"position: relative;width: 100%;padding-top: 12px;\" class=\"clearfix\">\n" +
    "            <div class=\"pull-left\" style=\"width: 300px;\">\n" +
    "                <input type=\"text\" class=\"form-control skinny\" placeholder=\"Search Users\" ng-model=\"userQuery\"/>\n" +
    "            </div>\n" +
    "            <div class=\"pull-right\">\n" +
    "                <a>Add User(s)</a>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "\n" +
    "        <div style=\"display: flex;padding-top: 12px;\">\n" +
    "            <div style=\"padding-right: 8px;\">\n" +
    "                <fui-dropdown skinny=\"true\" type=\"action\" items=\"actionItems\" default-label=\"actionLabel\" callback=\"actionSelected\" background=\"none\" color=\"black\" border=\"gray\"></fui-dropdown>\n" +
    "            </div>\n" +
    "            <div style=\"font-size: 0.6em;\">\n" +
    "                <div>for 0</div>\n" +
    "                <div>selected users</div>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "\n" +
    "        <table class=\"default\">\n" +
    "            <thead>\n" +
    "                <th><input type=\"checkbox\" /></th>\n" +
    "                <th>Name</th>\n" +
    "                <th>Tenant</th>\n" +
    "                <th>Role</th>\n" +
    "                <th>Access</th>\n" +
    "                <th>Last Activity</th>\n" +
    "            </thead>\n" +
    "            <tboday>\n" +
    "                <tr ng-repeat=\"user in users | filter:userQuery\">\n" +
    "                    <td><input type=\"checkbox\" /></td>\n" +
    "                    <td>{{user.firstname}} {{user.lastname}}</td>\n" +
    "                    <td>{{user.tenant}}</td>\n" +
    "                    <td>{{user.role}}</td>\n" +
    "                    <td>{{user.access}}</td>\n" +
    "                    <td>{{user.last_activity}}</td>\n" +
    "                </tr>\n" +
    "            </tboday>\n" +
    "        </table>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/volumes/create/nameType.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/volumes/create/nameType.html",
    "<div class=\"name-type\" ng-controller=\"nameTypeController\">\n" +
    "    <section>\n" +
    "        <div class=\"header\">\n" +
    "            <span class=\"light-print\">Volume Name</span>\n" +
    "        </div>\n" +
    "        <div class=\"volume-settings-panel\">\n" +
    "            <div class=\"volume-option form-group\">\n" +
    "                <input type=\"text\" class=\"volume-name form-control\" size=\"32\" placeholder=\"Enter Volume Name\" ng-model=\"name\">\n" +
    "                <!--<div class=\"clear-icon\" ng-click=\"name = ''\">\n" +
    "                </div>-->\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </section>\n" +
    "\n" +
    "    <section>\n" +
    "        <div class=\"header\">\n" +
    "            <span class=\"light-print\">Data Connector</span>\n" +
    "        </div>\n" +
    "\n" +
    "        <div class=\"volume-data-type-summary\" ng-show=\"!editing\">\n" +
    "            <div class=\"form-group inline\">\n" +
    "                <span>Data Type:</span>\n" +
    "                <span class=\"value\">{{data_connector.type}}</span>\n" +
    "            </div>\n" +
    "            <div class=\"form-group inline\" ng-show=\"data_connector.attributes.size\">\n" +
    "                <span>Capacity:</span>\n" +
    "                <span class=\"value\">{{data_connector.attributes.size}} {{data_connector.attributes.unit}}</span>\n" +
    "            </div>\n" +
    "            <div class=\"inline icon-container\">\n" +
    "                <span class=\"icon fui-new\" ng-click=\"editing = true\" title=\"Edit settings\"></span>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "\n" +
    "        <div class=\"volume-data-type-panel\" ng-class=\"{open: editing}\">\n" +
    "            <table class=\"default volumes\">\n" +
    "                <thead>\n" +
    "                    <th>Data Type</th>\n" +
    "                    <th>API</th>\n" +
    "                    <th>Capacity<br />(if Applicable)</th>\n" +
    "                    <th></th>\n" +
    "                </thead>\n" +
    "                <tbody id=\"data-connectors\">\n" +
    "                    <tr ng-repeat=\"connector in connectors\" ng-click=\"setSelected( connector );\">\n" +
    "                        <td>{{connector.type}}</td>\n" +
    "                        <td>{{connector.api}}</td>\n" +
    "                        <td>\n" +
    "                            <span class=\"connector-attributes\" ng-show=\"connector.attributes && connector.attributes.size\">\n" +
    "                                <button class=\"btn btn-primary skinny\" ng-click=\"editConnector( connector )\" ng-show=\"editingConnector != connector\">\n" +
    "                                    {{connector.attributes.size}}\n" +
    "                                    {{connector.attributes.unit}}\n" +
    "                                </button>\n" +
    "                                <div ng-show=\"editingConnector == connector\">\n" +
    "                                    <div class=\"inline vertical-middle\">\n" +
    "                                        <spinner value=\"$parent._selectedSize\" min=\"1\" max=\"999\" step=\"1\" class=\"skinny\"></spinner>\n" +
    "                                    </div>\n" +
    "                                    <div class=\"inline\">\n" +
    "                                        <fui-dropdown skinny=\"true\" background-color=\"#2486F8\" items=\"sizes\" selected=\"$parent._selectedUnit\"></fui-dropdown>\n" +
    "                                    </div>\n" +
    "                                    <button class=\"btn btn-primary skinny\" ng-click=\"saveConnectorChanges( connector )\">Save</button>\n" +
    "                                    <button class=\"btn btn-secondary skinny\" ng-click=\"stopEditing()\">Cancel</button>\n" +
    "                                </div>\n" +
    "                            </span>\n" +
    "                        </td>\n" +
    "                        <td class=\"checkmark\">\n" +
    "                            <span ng-class=\"{'fui-check': data_connector == connector}\"></span>\n" +
    "                        </td>\n" +
    "                    </tr>\n" +
    "                </tbody>\n" +
    "            </table>\n" +
    "            <div class=\"pull-right\">\n" +
    "                <button class=\"btn btn-primary\" ng-click=\"editing = false\">Done</button>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "    </section>\n" +
    "\n" +
    "<!--\n" +
    "    <p class=\"fine-print\">\n" +
    "    The key policy determines the initial policy that will be used. The master key will be assigned upon creation of this volume. You can then modify the policy or create additional keys with different policies.\n" +
    "    </p>\n" +
    "-->\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/volumes/create/qualityOfService.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/volumes/create/qualityOfService.html",
    "<div ng-controller=\"qualityOfServiceController\" class=\"volume-edit-row qos-panel\">\n" +
    "\n" +
    "        <div class=\"header\">\n" +
    "            <span class=\"light-print\">Quality of Service</span>\n" +
    "        </div>\n" +
    "\n" +
    "    <div class=\"volume-qos-summary-container\" ng-show=\"!editing\">\n" +
    "        <div class=\"form-group inline\">\n" +
    "            <span>Priority: </span>\n" +
    "            <span class=\"value\">{{priority}}</span>\n" +
    "        </div>\n" +
    "        <div class=\"form-group inline\">\n" +
    "            <span>IOPs Capacity Gaurantee: </span>\n" +
    "            <span class=\"value\" ng-show=\"capacity != 0\">{{capacity}}%</span>\n" +
    "            <span class=\"value\" ng-show=\"capacity == 0\">None</span>\n" +
    "        </div>\n" +
    "        <div class=\"form-group inline\">\n" +
    "            <span>IOPs Limit: </span>\n" +
    "            <span class=\"value\">{{iopLimit}} IOPs</span>\n" +
    "        </div>\n" +
    "        <div class=\"inline icon-container\" ng-click=\"editing = true\" title=\"Edit Settings\">\n" +
    "            <span class=\"icon fui-new\"></span>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "\n" +
    "    <div class=\"volume-edit-container\" ng-class=\"{open: editing}\">\n" +
    "        <section>\n" +
    "            <div>\n" +
    "                <span class=\"light-print\">Priority:</span>\n" +
    "                <span class=\"volume-edit-label\">{{priority}}</span>\n" +
    "            </div>\n" +
    "            <fui-slider values=\"priorityOptions\" value=\"priority\" min-label=\"Low\" max-label=\"High\"></fui-slider>\n" +
    "        </section>\n" +
    "        <section>\n" +
    "            <div style=\"margin-bottom: 8px;\">\n" +
    "                <span class=\"light-print\">IOPs Capacity Guarantee:</span>\n" +
    "                <span class=\"volume-edit-label\" ng-show=\"capacity != 0\">{{capacity}}%</span>\n" +
    "                <span class=\"volume-edit-label\" ng-show=\"capacity == 0\">None</span>\n" +
    "            </div>\n" +
    "            <fui-slider min=\"0\" max=\"100\" step=\"10\" value=\"capacity\" min-label=\"0%\" max-label=\"100%\"></fui-slider>\n" +
    "        </section>\n" +
    "        <section>\n" +
    "            <div style=\"margin-bottom: 8px;\">\n" +
    "                <span class=\"light-print\">IOPs Limit:</span>\n" +
    "                <span class=\"volume-edit-label\">{{iopLimit}}</span>\n" +
    "            </div>\n" +
    "            <fui-slider min=\"0\" max=\"3000\" step=\"100\" value=\"iopLimit\" min-label=\"None\" max-label=\"3000\"></fui-slider>\n" +
    "        </section>\n" +
    "        <div class=\"pull-right\">\n" +
    "            <button class=\"btn btn-primary\" ng-click=\"editing = false\">Done</button>\n" +
    "        </div>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/volumes/create/volumeCreate.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/volumes/create/volumeCreate.html",
    "<div class=\"create-panel volumes\" ng-controller=\"volumeCreateController\" ng-class=\"{shown: $parent.creating,hideit: !$parent.creating}\">\n" +
    "    <div class=\"modal-header\">\n" +
    "        <span>Create Volume</span>\n" +
    "    </div>\n" +
    "\n" +
    "    <div class=\"settings-body\">\n" +
    "        <div ng-include=\"'scripts/volumes/create/nameType.html'\"></div>\n" +
    "        <div ng-include=\"'scripts/volumes/create/qualityOfService.html'\"></div>\n" +
    "    </div>\n" +
    "\n" +
    "    <div class=\"pull-left\"style=\"padding-top: 12px;margin-bottom: 42px;\">\n" +
    "        <button class=\"btn btn-primary\" style=\"display: inline-block;\" ng-click=\"save()\">Create Volume</button>\n" +
    "        <button class=\"btn btn-secondary cancel-creation\" style=\"display: inline-block;\" ng-click=\"cancel()\">Cancel</button>\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);

angular.module("scripts/volumes/volumes.html", []).run(["$templateCache", function($templateCache) {
  $templateCache.put("scripts/volumes/volumes.html",
    "<div ng-controller=\"volumeController\">\n" +
    "\n" +
    "    <div  class=\"volumeparent\" ng-style=\"{left: creating ? '-100%' : '0%'}\">\n" +
    "\n" +
    "        <!-- if it's not in a table the TR elements are deleted.  If it's not outside of\n" +
    "            the real table it sometimes gets removed from the DOM by angular -->\n" +
    "        <table>\n" +
    "            <tbody id=\"volume-temp-table\">\n" +
    "                <tr id=\"volume-edit-row\" class=\"volume-edit-row\" ng-show=\"editing\">\n" +
    "                    <td colspan=\"100%\">\n" +
    "                        <div class=\"volume-edit-container\" ng-class=\"{open: editing}\">\n" +
    "                            <section>\n" +
    "                                <div>\n" +
    "                                    <span class=\"light-print\">Priority:</span>\n" +
    "                                    <span class=\"volume-edit-label\">{{priority}}</span>\n" +
    "                                </div>\n" +
    "                                <fui-slider values=\"priorityOptions\" value=\"priority\" min-label=\"Low\" max-label=\"High\"></fui-slider>\n" +
    "                            </section>\n" +
    "                            <section>\n" +
    "                                <div style=\"margin-bottom: 8px;\">\n" +
    "                                    <span class=\"light-print\">IOPs Capacity Guarantee:</span>\n" +
    "                                    <span class=\"volume-edit-label\" ng-show=\"capacity != 0\">{{capacity}}%</span>\n" +
    "                                    <span class=\"volume-edit-label\" ng-show=\"capacity == 0\">None</span>\n" +
    "                                </div>\n" +
    "                                <fui-slider min=\"0\" max=\"100\" step=\"10\" value=\"capacity\" min-label=\"None\" max-label=\"100%\"></fui-slider>\n" +
    "                            </section>\n" +
    "                            <section>\n" +
    "                                <div style=\"margin-bottom: 8px;\">\n" +
    "                                    <span class=\"light-print\">IOPs Limit:</span>\n" +
    "                                    <span class=\"volume-edit-label\">{{iopLimit}}</span>\n" +
    "                                </div>\n" +
    "                                <fui-slider min=\"0\" max=\"3000\" step=\"100\" value=\"iopLimit\" min-label=\"None\" max-label=\"3000\"></fui-slider>\n" +
    "                            </section>\n" +
    "                        </div>\n" +
    "                        <div class=\"pull-right\">\n" +
    "                            <button class=\"btn btn-primary\" ng-click=\"save()\">Save</button>\n" +
    "                            <button class=\"btn btn-secondary\" ng-click=\"cancel()\">Cancel</button>\n" +
    "                        </div>\n" +
    "                    </td>\n" +
    "                </tr>\n" +
    "            </tbody>\n" +
    "        </table>\n" +
    "\n" +
    "        <!-- Real stuff -->\n" +
    "        <div class=\"volumecontainer\">\n" +
    "\n" +
    "            <div class=\"header volumes\">\n" +
    "                <span>{{name}}</span>\n" +
    "                <span>Volumes</span>\n" +
    "                <a ng-click=\"createNewVolume()\" class=\"pull-right new_volume\">Create a Volume</a>\n" +
    "            </div>\n" +
    "\n" +
    "            <div class=\"col-xs-12\">\n" +
    "                <!-- actual table -->\n" +
    "                <div class=\"table-container\">\n" +
    "                    <table class=\"default volumes clickable\" style=\"margin-bottom: 48px;\">\n" +
    "                        <thead>\n" +
    "                            <th>Performance</th>\n" +
    "                            <th>Name</th>\n" +
    "                            <th>Data Type</th>\n" +
    "                            <th>\n" +
    "                                <div>Capacity</div>\n" +
    "                                <div>Used/Limit</div>\n" +
    "                            </th>\n" +
    "                            <th>IOPs Capacity Garauntee</th>\n" +
    "                            <th class=\"priority\">Priority</th>\n" +
    "                            <th></th>\n" +
    "                        </thead>\n" +
    "                        <tbody>\n" +
    "                            <!-- row per volume -->\n" +
    "                            <tr class=\"volume-row\" ng-repeat=\"volume in volumes\">\n" +
    "                                <td class=\"volume_time\">\n" +
    "                                    <div></div>\n" +
    "                                </td>\n" +
    "\n" +
    "                                <td class=\"name primary-color\">\n" +
    "                                    {{volume.name}}\n" +
    "                                </td>\n" +
    "                                <td>\n" +
    "                                    {{volume.data_connector.type}}\n" +
    "                                </td>\n" +
    "\n" +
    "                                <td class=\"consume\">\n" +
    "                                    {{volume.current_usage.size}}{{volume.current_usage.unit}}\n" +
    "                                    <span>/</span>\n" +
    "                                    {{volume.data_connector.attributes.size}}{{volume.data_connector.attributes.unit}}\n" +
    "                                </td>\n" +
    "\n" +
    "                                <td class=\"iopLimit\">\n" +
    "                                    <span ng-show=\"volume.sla != 0\">{{ volume.sla }}%</span>\n" +
    "                                    <span ng-show=\"volume.sla == 0\">None</span>\n" +
    "                                </td>\n" +
    "\n" +
    "                                <td class=\"priority\">\n" +
    "                                    <em>\n" +
    "                                        {{volume.priority}}\n" +
    "                                    </em>\n" +
    "                                </td>\n" +
    "                                <td class=\"button-group\">\n" +
    "                                    <div class=\"inline\">\n" +
    "                                        <span class=\"fui-new\" ng-click=\"edit( $event, volume )\" title=\"Edit settings\"></span>\n" +
    "                                        <!--<span class=\"fui-cross\" ng-click=\"delete( volume )\" title=\"Delete volume\"></span>-->\n" +
    "                                    </div>\n" +
    "                                </td>\n" +
    "                            </tr>\n" +
    "                            <tr class=\"disabled\" ng-show=\"!volumes.length || volumes.length === 0\">\n" +
    "                                <td colspan=\"100%\">\n" +
    "                                    There are no registered volumes.\n" +
    "                                </td>\n" +
    "                            </tr>\n" +
    "                        </tbody>\n" +
    "                    </table>\n" +
    "                </div>\n" +
    "            </div>\n" +
    "        </div>\n" +
    "\n" +
    "        <!-- policy editing slide out... for now... -->\n" +
    "<!--        <div ng-include=\"'scripts/volumes/volumeEdit.html'\"></div>   -->\n" +
    "\n" +
    "        <!-- volume creation slide out -->\n" +
    "        <div ng-include=\"'scripts/volumes/create/volumeCreate.html'\"></div>\n" +
    "\n" +
    "        <!-- volume drill in -->\n" +
    "\n" +
    "\n" +
    "    </div>\n" +
    "</div>\n" +
    "");
}]);
