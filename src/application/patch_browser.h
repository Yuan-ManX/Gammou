#ifndef GAMMOU_PATCH_BROWSER_H
#define GAMMOU_PATCH_BROWSER_H

#include "application.h"

namespace Gammou {

    std::unique_ptr<View::widget> make_patch_browser(application& app);

}

#endif