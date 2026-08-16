#include "gameplay/medal.h"

namespace flappy {
    Medal medal_for_score(int score)
    {
        if(score >= kPlatinumScore)
        {
            return Medal::Platinum;
        }

        if(score >= kGoldScore)
        {
            return Medal::Gold;
        }

        if(score >= kSilverScore)
        {
            return Medal::Silver;
        }

        if(score >= kBronzeScore)
        {
            return Medal::Bronze;
        }

        return Medal::None;
    }

    const char* medal_name(Medal medal)
    {
        switch (medal)
        {
            case Medal::Bronze :
                return "BRONZE";
            
            case Medal::Silver:
                return "SILVER";

            case Medal::Gold:
                return "GOLD";

            case Medal::Platinum:
                return "PLATINUM";

            case Medal::None:
                return "";
        }
        return "";
    }
}