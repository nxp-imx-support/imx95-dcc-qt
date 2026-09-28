/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "kill_me_model.h"

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_kill, "model.kill_sw")

KillMeModel::KillMeModel(QObject* parent) : QObject(parent)
{

}

KillMeModel::~KillMeModel()
{

}

KillMeModel* KillMeModel::instance()
{
    static std::unique_ptr<KillMeModel> s_instance = nullptr;
    if(!s_instance)
    {
        s_instance.reset(new KillMeModel);
    }
    return s_instance.get();
}

void KillMeModel::killMe(void)
{
    /* Check ENV to determine if cause a real panic */
    const char* env_name = "DCC_KILL_DEBUG";
    if (qEnvironmentVariableIsSet(env_name) &&
        qEnvironmentVariable(env_name) != QString("0")) {
        /* Just debug */
        qCDebug(lc_kill) << "DCC kill emulated";
    } else {
        /* Real panic */
        //runner.startCommand("/bin/bash -c \"echo 1 > /proc/sys/kernel/sysrq && echo c | tee /proc/sysrq-trigger\"");
        //runner.startCommand("/bin/bash -c \"poweroff\"");
        runner.startCommand("/bin/bash -c \"reboot\"");
        runner.waitForFinished();
    }
}
