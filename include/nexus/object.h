#ifndef NEXUS_OBJECT_H
#define NEXUS_OBJECT_H

#include <memory>
#include <vector>
#include <optional>


#include <nexus-api.h>
#include <nexus/property.h>

namespace nexus {

    namespace detail {

        // All Actual objects need an owner (except System)
        // + and ID within the owner
        class Impl {
        public:

            Impl(Impl *_owner = nullptr, nxs_int _id = -1)
                : owner(_owner), id(_id) {}
            virtual ~Impl() {}

            nxs_int getId() const { return id; }

        protected:
            // Only the derived class can access
            template <typename T = Impl>
            T *getParent() const { return dynamic_cast<T*>(owner); }

            template <typename T>
            T *getParentOfType() const {
                if (auto *par = dynamic_cast<T*>(owner))
                    return par;
                if (owner)
                    return owner->getParentOfType<T>();
                return nullptr;
            }

        private:
            Impl *owner;
            nxs_int id;
        };
    }

    // Facade base-class
    template <typename Timpl, typename Towner = void>
    class Object {
        // set of runtimes
        typedef std::shared_ptr<Timpl> ImplRef;
        ImplRef impl;
    
    public:

        typedef Towner OwnerTy;
        
        template <typename... Args>
        Object(detail::Impl owner, Args... args) : impl(std::make_shared<Timpl>(owner, args...)) {}

        template <typename... Args>
        Object(Args... args) : impl(std::make_shared<Timpl>(args...)) {}

        // Empty CTor - assumes Impl doesn't have an empty CTOR
        Object() = default;
        virtual ~Object() {}

        operator bool() const { return impl && nxs_valid_id(getId()); }
        bool operator ==(const Object &that) const { return impl == that.impl; }

        virtual void release() { impl = nullptr; }

        virtual nxs_int getId() const = 0;

        virtual std::optional<Property> getProperty(nxs_int prop) const = 0;

        template <typename T>
        const T getProp(nxs_int prop) const {
            if (auto val = getProperty(prop))
                return getPropertyValue<T>(*val);
            return T();
        }

    protected:
        ImplRef get() const { return impl; }
    };
    

    // Storage of vector of objects
    template <typename Tobject>
    class Objects {
        // set of runtimes
        typedef std::vector<Tobject> ObjectVec;
        std::shared_ptr<ObjectVec> objects;
    
    public:
        Objects() : objects(std::make_shared<ObjectVec>()) {}

        nxs_int size() const {
            return objects->size();
        }
        nxs_int add(Tobject obj) {
            objects->push_back(obj);
            return objects->size() - 1;
        }
        Tobject get(nxs_int idx) const {
            if (idx >= 0 && idx < objects->size())
                return (*objects)[idx];
            return Tobject();
        }
        void clear() {
            objects->clear();
        }

        typename ObjectVec::iterator begin() const { return objects->begin(); }
        typename ObjectVec::iterator end() const { return objects->end(); }
    };
    
}

#endif // NEXUS_OBJECT_H
