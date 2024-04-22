class EventBus
{
public:
    using LuaRef = luabridge::LuaRef;
    using Subscription = std::pair<LuaRef, LuaRef>; // Component, Function (used to track lua functions and their tables)
    using SubscriberList = std::vector<Subscription>;
    using DeferredAction = std::function<void()>;

    static inline std::map<std::string, SubscriberList> subscribers;
    static inline std::vector<DeferredAction> deferredActions;

    EventBus() = default;

    static void Publish(const std::string &event_type, LuaRef event_object)
    {
        if (subscribers.find(event_type) != subscribers.end())
        {
            for (auto &[component, function]: subscribers[event_type])
            {
                // Important: Ensure 'self' is correctly passed to Lua functions
                if (function.isFunction())
                {
                    // Call the Lua function with 'component' as 'self' and any additional parameters
                    function(component, event_object);
                }
            }
        }
    }

    static void Subscribe(const std::string &event_type, LuaRef component, LuaRef function)
    {
        // Ensure that 'function' is actually a function.
        deferredActions.emplace_back([=]()
                                     {
                                         subscribers[event_type].emplace_back(component, function);
                                     });
    }

    static void UnSubscribe(const std::string &event_type, LuaRef component, LuaRef function)
    {
        deferredActions.emplace_back([=]()
                                     {
                                         auto &list = subscribers[event_type];
                                         list.erase(
                                                 std::remove_if(list.begin(), list.end(), [&](const Subscription &sub)
                                                 {
                                                     return sub.first == component && sub.second == function;
                                                 }), list.end());
                                     });
    }

    static void ProcessDeferredActions()
    {
        for (auto &action: deferredActions)
        {
            action(); // Execute each deferred action
        }
        deferredActions.clear(); // Clear the list of deferred actions
    }
};
